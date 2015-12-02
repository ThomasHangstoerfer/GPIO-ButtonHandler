
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <syslog.h>
#include <time.h>

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
 
	return 0;
}

int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd;
	char buf[MAX_BUF];
 
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}
 
	if (out_flag)
		write(fd, "out", 4);
	else
		write(fd, "in", 3);
 
	close(fd);
	return 0;
}

int gpio_set_value(unsigned int gpio, unsigned int value)
{
	int fd;
	char buf[MAX_BUF];
 
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-value");
		return fd;
	}
 
	if (value)
		write(fd, "1", 2);
	else
		write(fd, "0", 2);
 
	close(fd);
	return 0;
}

int gpio_get_value(unsigned int gpio, unsigned int *value)
{
	int fd;
	char buf[MAX_BUF];
	char ch;

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror("gpio/get-value");
		return fd;
	}
 
	read(fd, &ch, 1);

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}
 
	close(fd);
	return 0;
}


int gpio_set_edge(unsigned int gpio, char *edge)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-edge");
		return fd;
	}
 
	write(fd, edge, strlen(edge) + 1); 
	close(fd);
	return 0;
}


int gpio_fd_open(unsigned int gpio)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_RDONLY | O_NONBLOCK );
	if (fd < 0) {
		perror("gpio/fd_open");
	}
	return fd;
}


int gpio_fd_close(int fd)
{
	return close(fd);
}


int main(int argc, char **argv, char **envp)
{
	struct pollfd fdset[3];
	int nfds = 3;
	int gpio_b1_fd, gpio_b2_fd, gpio_b3_fd;
	int timeout, rc;
	char *buf[MAX_BUF];
	int b1_first = 1;
	int b2_first = 1;
	int b3_first = 1;

	gpio_b1_fd = gpio_fd_open(20);
	gpio_b2_fd = gpio_fd_open(7);
	gpio_b3_fd = gpio_fd_open(106);

	timeout = POLL_TIMEOUT;
 
	while (1) {
		memset((void*)fdset, 0, sizeof(fdset));
		memset(buf, 0, MAX_BUF);

		//fdset[0].fd = STDIN_FILENO;
		//fdset[0].events = POLLIN;
      
		fdset[0].fd = gpio_b1_fd;
		fdset[0].events = POLLPRI;

		fdset[1].fd = gpio_b2_fd;
		fdset[1].events = POLLPRI;

		fdset[2].fd = gpio_b3_fd;
		fdset[2].events = POLLPRI;

		rc = poll(fdset, nfds, timeout);      

		if (rc < 0) {
			printf("\npoll() failed! %s\n", strerror(errno));
			return -1;
		}
      
		if (rc == 0) {
			// timeout occured
			//printf(".");
		}
		if (fdset[0].revents & POLLPRI) {
			printf("b1_first = %i\n", b1_first);
			read(fdset[0].fd, buf, MAX_BUF);
			if ( b1_first == 1 )
			{
				b1_first = 0;
			}
			else
			{
				printf("\npoll() GPIO %d interrupt occurred -> Button 1\n", 20);
				syslog(LOG_ERR, "* BUTTON 1 *");
				syslog(LOG_ERR, "* SHUTDOWN 1 *");
				//printf("buf[0] = %i\n", (int)buf[0]);

				// SHUTDOWN

				gpio_set_value(82, 0);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(82, 1);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(82, 0);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(82, 1);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(82, 0);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(82, 1);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(82, 0);

				system("/sbin/halt");
			}
		}
            
		if (fdset[1].revents & POLLPRI) {
			read(fdset[1].fd, buf, MAX_BUF);
			if ( b2_first == 1 )
			{
				b2_first = 0;
			}
			else
			{
				printf("\npoll() GPIO %d interrupt occurred -> Button 2\n", 7);
				syslog(LOG_ERR, "* BUTTON 2 *");
				syslog(LOG_ERR, "* REBOOT *");

				gpio_set_value(83, 0);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(83, 1);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(83, 0);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(83, 1);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(83, 0);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(83, 1);
				nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
				gpio_set_value(83, 0);

				system("/sbin/reboot");

			}
		}

		if (fdset[2].revents & POLLPRI) {
			read(fdset[2].fd, buf, MAX_BUF);
			if ( b3_first == 1 )
			{
				b3_first = 0;
			}
			else
			{
				printf("\npoll() GPIO %d interrupt occurred -> Button 3\n", 106);
				syslog(LOG_ERR, "* BUTTON 3 *");
				syslog(LOG_ERR, "* NO ACTION *");
			}
		}

		//fflush(stdout);
	}

	gpio_fd_close(gpio_b1_fd);
	gpio_fd_close(gpio_b2_fd);
	gpio_fd_close(gpio_b3_fd);
	return 0;
}
