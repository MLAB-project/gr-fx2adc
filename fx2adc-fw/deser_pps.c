#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

/* plagiarized from plan9port */
static long readn(int f, void *av, long n)
{
	char *a;
	long m, t;

	a = av;
	t = 0;
	while(t < n){
		m = read(f, a+t, n-t);
		if(m <= 0){
			if(t == 0)
				return m;
			break;
		}
		t += m;
	}
	return t;
}

static ssize_t writen(int fd, const void *buf, size_t count)
{
	size_t written;
	ssize_t ret;

	written = 0;
	while (written < count) {
		ret = write(fd, ((char *) buf) + written, count - written);

		if (ret <= 0) {
			if (ret < 0 && errno == EINTR)
				continue;

			return ret;
		}

		written += ret;
	}

	return written;
}


char readbyte() {
	char b;
	assert(readn(0, &b, 1));
	return b;
}

int main(int argc, char const *argv[])
{
	int i, s, x;
	char buff[2048];

	for (i = 0; i < 1000; i++)	
		readn(0, buff, sizeof(buff));

	while (!(readbyte() & 0x80));

	i = 0;
	while (readbyte() & 0x80) i++;
	//fprintf(stderr, "%d\n", i);
	assert(i == 31);

	i = 0;
	while (!(readbyte() & 0x80)) i++;
	//fprintf(stderr, "%d\n", i);
	assert(i == 31);

	for (i = 0; i < 31; i++) { 
		if (!(readbyte() & 0x80))
			perror("out of sync");
	}

	for (i = 0; i < 32; i++) { 
		if (readbyte() & 0x80)
			perror("out of sync");
	}

	for (i = 0; i < 32; i++) { 
		if (!(readbyte() & 0x80))
			perror("out of sync");
	}

	fprintf(stderr, "synced\n");

	int prev = 0;

	uint32_t samples[sizeof(buff) / 32];
	while (1) {
		assert(readn(0, buff, sizeof(buff)) == sizeof(buff));

		int rl = 0;

		for (i = 0; i < sizeof(buff); i += 32) {
			uint32_t smp = 0;

			for (s = 0; s < 32; s++) {
				char b = buff[i + s];
				assert(((b & 0x80) != 0) == rl);

				if ((b & 0x40) && !prev)
					fprintf(stderr, "pps\n");

				prev = b & 0x40;
				
				smp <<= 1;
				if (b & 0x01)
					smp |= 1;
			}

			//if (smp & 0x00800000)
			//	smp |= 0xff000000;

			samples[i / 32] = smp;
			rl = !rl;
		}

		writen(1, samples, sizeof(samples));
	}
}
