 /*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glib.h>
#include <Ecore.h>
#include <tizen.h>
#include <service_app.h>
#include <camera.h>
#include <mv_common.h>
#include <pthread.h>
#include "controller.h"
#include "controller_mv.h"
#include "controller_image.h"
#include "log.h"
#include "resource_camera.h"
#include "1602A_lcd_util.h"
#include "resource/resource_joystick.h"
#include "resource/resource_joystick_internal.h"
#include "resource_servo_motor_sg90.h"
#include "servo-type.h"
#include "memory.h"
#include <stdlib.h>
#define CAMERA_MOVE_INTERVAL_MS 450
#define THRESHOLD_VALID_EVENT_COUNT 2
#define VALID_EVENT_INTERVAL_MS 200

#define IMAGE_FILE_PREFIX "CAM_"
#define EVENT_INTERVAL_SECOND 0.5f

//#define TEMP_IMAGE_FILENAME "/opt/usr/home/owner/apps_rw/org.tizen.smart-surveillance-camera/shared/data/tmp.jpg"
//#define LATEST_IMAGE_FILENAME "/opt/usr/home/owner/apps_rw/org.tizen.smart-surveillance-camera/shared/data/latest.jpg"

// #define ENABLE_SMARTTHINGS
#define APP_CALLBACK_KEY "controller"

typedef struct app_data_s {
	double current_servo_x;
	double current_servo_y;
	int motion_state;

	long long int last_moved_time;
	long long int last_valid_event_time;
	int valid_vision_result_x_sum;
	int valid_vision_result_y_sum;
	int valid_event_count;

	unsigned int latest_image_width;
	unsigned int latest_image_height;
	char *latest_image_info;
	int latest_image_type; // 0: image during camera repositioning, 1: single valid image but not completed, 2: fully validated image
	unsigned char *latest_image_buffer;

	Ecore_Thread *image_writter_thread;
	pthread_mutex_t mutex;

	char* temp_image_filename;
	char* latest_image_filename;
} app_data;
typedef struct loc_data_s{
	double loc;
	int idx;
} loc_data;
char s[100];
int f = 1, pano_cnt = 0;
double loc = 1.0f;
pthread_t p1;
char* shared_data_path;
unsigned char *pre_buffer;


static void __thread_write_image_file(void *data, Ecore_Thread *th)
{
	app_data *ad = (app_data *)data;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned char *buffer = 0;
	char *image_info = NULL;
	int ret = 0;

	pthread_mutex_lock(&ad->mutex);
	width = ad->latest_image_width;
	height = ad->latest_image_height;
	buffer = ad->latest_image_buffer;
	//_D("buf : %s", buffer);
	ad->latest_image_buffer = NULL;
	if (ad->latest_image_info) {
		image_info = ad->latest_image_info;
		ad->latest_image_info = NULL;
	} else {
		image_info = strdup("00");
	}
	pthread_mutex_unlock(&ad->mutex);
	ret = controller_image_save_image_file(ad->temp_image_filename, width, height, buffer, image_info, strlen(image_info));
	if (ret) {
		_E("failed to save image file");
	} else {
		ret = rename(ad->temp_image_filename, ad->latest_image_filename);
		if (ret != 0 )
			_E("Rename fail");
	}
	free(image_info);
	//free(buffer);
	pthread_mutex_lock(&ad->mutex);
	if(pre_buffer != NULL) {
		free(pre_buffer);
		pre_buffer = NULL;
	}
	pre_buffer = buffer;
	pthread_mutex_unlock(&ad->mutex);
}

static void __thread_end_cb(void *data, Ecore_Thread *th)
{
	app_data *ad = (app_data *)data;

	pthread_mutex_lock(&ad->mutex);
	ad->image_writter_thread = NULL;
	pthread_mutex_unlock(&ad->mutex);
}

static void __thread_cancel_cb(void *data, Ecore_Thread *th)
{
	app_data *ad = (app_data *)data;
	unsigned char *buffer = NULL;

	_E("Thread %p got cancelled.\n", th);
	pthread_mutex_lock(&ad->mutex);
	buffer = ad->latest_image_buffer;
	ad->latest_image_buffer = NULL;
	ad->image_writter_thread = NULL;
	pthread_mutex_unlock(&ad->mutex);

	free(buffer);
}

static void __copy_image_buffer(image_buffer_data_s *image_buffer, app_data *ad)
{
	unsigned char *buffer = NULL;

	pthread_mutex_lock(&ad->mutex);
	ad->latest_image_height = image_buffer->image_height;
	ad->latest_image_width = image_buffer->image_width;

	buffer = ad->latest_image_buffer;
	ad->latest_image_buffer = image_buffer->buffer;
	pthread_mutex_unlock(&ad->mutex);

	free(buffer);
}

static void __preview_image_buffer_created_cb(void *data)
{
	image_buffer_data_s *image_buffer = data;
	app_data *ad = (app_data *)image_buffer->user_data;
	//_D("buffsize : %u, f : %d", image_buffer->buffer_size, image_buffer->format);

	ret_if(!image_buffer);
	ret_if(!ad);

	__copy_image_buffer(image_buffer, ad);

	pthread_mutex_lock(&ad->mutex);
	if (!ad->image_writter_thread) {
		ad->image_writter_thread = ecore_thread_run(__thread_write_image_file, __thread_end_cb, __thread_cancel_cb, ad);
	} else {
		_E("Thread is running NOW");
	}
	pthread_mutex_unlock(&ad->mutex);

	return;

FREE_ALL_BUFFER:
	free(image_buffer->buffer);
	free(image_buffer);
}


static void __device_interfaces_fini(void)
{
	if(p1 != NULL){
		f= 0;
		pthread_join(p1, NULL);
	}
	resource_1602A_LCD_close(0);
	resource_joystick_close();
}

static int __device_interfaces_init(app_data *ad)
{
	retv_if(!ad, -1);
	resource_set_1602A_LCD_configuration(0, 2, 16, 4, 46, 130, 26, 25, 27, 0, 0, 0, 0, 0);
	return 0;

ERROR :
	__device_interfaces_fini();
	return -1;
}
unsigned char save[5][115200];
unsigned char buff[115200*5];
int diff_rotate[5];
loc_data save_loc[5];
void _write_image_file(void *data)
{
	app_data *ad = (app_data *)data;
	unsigned int width = 0;
	unsigned int height = 0;
	char *image_info = NULL;
	char *pano_path;
	char p_name[20];
	int ret = 0;
	pthread_mutex_lock(&ad->mutex);
	width = ad->latest_image_width;
	height = ad->latest_image_height;
	memcpy(save[pano_cnt], pre_buffer, 115200);
	ad->latest_image_buffer = NULL;
	if (ad->latest_image_info) {
		image_info = ad->latest_image_info;
		ad->latest_image_info = NULL;
	} else {
		image_info = strdup("00");
	}
	save_loc[pano_cnt].loc = loc;
	save_loc[pano_cnt].idx = pano_cnt;
	sprintf(p_name, "%d.jpg", pano_cnt);
	pano_cnt++;
	pano_path = g_strconcat(shared_data_path, p_name, NULL);
	//_D("pano : %s, w: %d, h : %d", pano_path, width, height);
	ret = controller_image_save_image_file(pano_path, width, height, pre_buffer, image_info, strlen(image_info));
	if (ret) {
		_E("failed to save image file");
	}
	pthread_mutex_unlock(&ad->mutex);
	free(image_info);
}
int cmp(const void* a, const void *b){
	const loc_data *m, *n;
	m = (loc_data*)a;
	n = (loc_data*)b;
	if(m->loc < n->loc) return -1;
	else if(m->loc == n->loc) return 0;
	else return 1;
}
void __thread_servo(void *data){
	app_data *ad = (app_data *)data;
	int tx,ty,sw, idx = 0, ret = 0;
	char *pano_path, *image_info;
	char *p_name = "pano.jpg";
	while(f){
		resource_joystick_read(&tx, &ty, &sw);
		if(sw > 300){
			_write_image_file(ad);
			_D("cnt : %d", pano_cnt);
			if(pano_cnt == 5){
				qsort(save_loc, 5, sizeof(loc_data), cmp);
				for(int i=1; i<5; i++) {
					diff_rotate[i] = (int)((save_loc[i].loc - save_loc[i-1].loc)*10);
					_D("%d diff : %d", i, diff_rotate[i]);
				}
				for(int k=0; k<240; k++){
					for(int i=0; i<4; i++){
						for(int j=0; j<80*diff_rotate[i+1]; j++){
							buff[idx++] = save[save_loc[i].idx][j+k*320];
						}
					}
					for(int j=0; j<320; j++){
						buff[idx++] = save[save_loc[4].idx][j+k*320];
					}
				}
				for(int k=0; k<120; k++){
					for(int i=0; i<4; i++){
						for(int j=0; j<40*diff_rotate[i+1]; j++){
							buff[idx++] = save[save_loc[i].idx][j+k*160 + 76800];
						}
					}
					for(int j=0; j<160; j++){
						buff[idx++] = save[save_loc[4].idx][j+k*160 + 76800];
					}
				}
				for(int k=0; k<120; k++){
					for(int i=0; i<4; i++){
						for(int j=0; j<40*diff_rotate[i+1]; j++){
							buff[idx++] = save[save_loc[i].idx][j+k*160 + 76800+19200];
						}
					}
					for(int j=0; j<160; j++){
						buff[idx++] = save[save_loc[4].idx][j+k*160 + 76800+19200];
					}
				}
				image_info = strdup("00");
				pano_path = g_strconcat(shared_data_path, p_name, NULL);
				int t = 0;
				for(int i=0; i<4; i++) t += (80*diff_rotate[i+1]);
				ret = controller_image_save_image_file(pano_path, 320+t, 240, buff, image_info, strlen(image_info));
				if (ret) {
					_E("failed to save image file");
				}
				free(image_info);
				idx = pano_cnt = 0;
			}
		}
		else if(tx < 10){
			loc += 0.1f;
			if(loc>2.3f) loc = 2.3f;
		}
		else if(tx>300){
			loc -= 0.1f;
			if(loc < 0.6f) loc = 0.6f;
		}
		resource_set_servo_motor_sg90_value(2, loc);
		sprintf(s, "Take a %d' Image", pano_cnt+1);
		print_lcd(s, 0);
		//_D("loc : %lf, x : %d, y : %d, sw : %d",loc, tx, ty, sw);
		resource_util_delay(500);
	}
}
static bool service_app_create(void *data)
{
	app_data *ad = (app_data *)data;
	unsigned char *decode = NULL;
	unsigned long long size = 0;
	shared_data_path = app_get_shared_data_path();
	if (shared_data_path == NULL) {
		_E("Failed to get shared data path");
		goto ERROR;
	}
	ad->temp_image_filename = g_strconcat(shared_data_path, "tmp.jpg", NULL);
	ad->latest_image_filename = g_strconcat(shared_data_path, "latest.jpg", NULL);

	_D("%s", ad->temp_image_filename);
	_D("%s", ad->latest_image_filename);

	controller_image_initialize();
	pthread_create(&p1, NULL, __thread_servo, ad);
	pthread_mutex_init(&ad->mutex, NULL);

	if (__device_interfaces_init(ad))
		goto ERROR;

	if (resource_camera_init(__preview_image_buffer_created_cb, ad) == -1) {
		_E("Failed to init camera");
		goto ERROR;
	}

	if (resource_camera_start_preview() == -1) {
		_E("Failed to start camera preview");
		goto ERROR;
	}

#ifdef ENABLE_SMARTTHINGS
	/* smartthings APIs should be called after camera start preview, they can't wait to start camera */
	if (st_thing_master_init())
		goto ERROR;

	if (st_thing_resource_init())
		goto ERROR;
#endif /* ENABLE_SMARTTHINGS */

	return true;

ERROR:
	__device_interfaces_fini();

	resource_camera_close();
	controller_mv_unset_movement_detection_event_cb();
	controller_image_finalize();

#ifdef ENABLE_SMARTTHINGS
	st_thing_master_fini();
	st_thing_resource_fini();
#endif /* ENABLE_SMARTTHINGS */

	pthread_mutex_destroy(&ad->mutex);

	return false;
}

static void service_app_terminate(void *data)
{
	app_data *ad = (app_data *)data;
	Ecore_Thread *thread_id = NULL;
	unsigned char *buffer = NULL;
	char *info = NULL;
	gchar *temp_image_filename;
	gchar *latest_image_filename;
	_D("App Terminated - enter");
	free(shared_data_path);
	if(pre_buffer != NULL) free(pre_buffer);
	resource_camera_close();
	controller_mv_unset_movement_detection_event_cb();
	pthread_mutex_lock(&ad->mutex);
	thread_id = ad->image_writter_thread;
	ad->image_writter_thread = NULL;
	pthread_mutex_unlock(&ad->mutex);

	if (thread_id)
		ecore_thread_wait(thread_id, 3.0); // wait for 3 second

	__device_interfaces_fini();

	controller_image_finalize();

#ifdef ENABLE_SMARTTHINGS
	st_thing_master_fini();
	st_thing_resource_fini();
#endif /* ENABLE_SMARTTHINGS */

	pthread_mutex_lock(&ad->mutex);
	buffer = ad->latest_image_buffer;
	ad->latest_image_buffer = NULL;
	info  = ad->latest_image_info;
	ad->latest_image_info = NULL;
	temp_image_filename = ad->temp_image_filename;
	ad->temp_image_filename = NULL;
	latest_image_filename = ad->latest_image_filename;
	ad->latest_image_filename = NULL;
	pthread_mutex_unlock(&ad->mutex);
	free(buffer);
	free(info);
	g_free(temp_image_filename);
	g_free(latest_image_filename);

	pthread_mutex_destroy(&ad->mutex);
	free(ad);
	_D("App Terminated - leave");
}

static void service_app_control(app_control_h app_control, void *data)
{
	/* APP_CONTROL */
}

int main(int argc, char* argv[])
{
	app_data *ad = NULL;
	int ret = 0;
	service_app_lifecycle_callback_s event_callback;

	ad = calloc(1, sizeof(app_data));
	retv_if(!ad, -1);

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	ret = service_app_main(argc, argv, &event_callback, ad);

	return ret;
}
