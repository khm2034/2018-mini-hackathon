#include "resource/resource_util.h"

void resource_util_delay_microseconds_hard (unsigned int how_long)
{
	struct timeval t_now, t_long, t_end ;

	gettimeofday (&t_now, NULL) ;
	t_long.tv_sec  = how_long / 1000000 ;
	t_long.tv_usec = how_long % 1000000 ;
	timeradd (&t_now, &t_long, &t_end) ;

	while (timercmp (&t_now, &t_end, <))
		gettimeofday (&t_now, NULL) ;
}

void resource_util_delay_microseconds (unsigned int how_long)
{
	struct timespec sleeper ;
	unsigned int u_secs = how_long % 1000000 ;
	unsigned int w_secs = how_long / 1000000 ;

	if (how_long ==   0)
		return ;
	else if (how_long  < 100)
		resource_util_delay_microseconds_hard(how_long);
	else
	{
		sleeper.tv_sec  = w_secs ;
		sleeper.tv_nsec = (long)(u_secs * 1000L) ;
		nanosleep (&sleeper, NULL) ;
	}
}
void resource_util_delay (unsigned int how_long){
	struct timespec sleeper, dummy ;

	sleeper.tv_sec  = (time_t)(how_long / 1000) ;
	sleeper.tv_nsec = (long)(how_long % 1000) * 1000000 ;

	nanosleep (&sleeper, &dummy) ;
}
