#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

struct posix_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
				/* 500 */
};

#define BLOCKSIZE 512

union block
{
	char buffer[BLOCKSIZE];
	struct posix_header header;
};

/* The checksum field is filled with this while the checksum is computed.  */
#define CHKBLANKS	"        "	/* 8 blanks, no null */

// Global buffer.
static union block g_buffer;

void simple_finish_header(union block *h)
{
	size_t i;
	int sum = 0;
	char *p;

	memcpy(h->header.chksum, CHKBLANKS, sizeof(h->header.chksum));

	sum = 0;
	p = h->buffer;
	for (i = sizeof *h; i-- != 0; ) {
		sum += 0xFF & *p++;
	}

	/* Fill in the checksum field.  It's formatted differently from the
	other fields: it has [6] digits, a null, then a space -- rather than
	digits, then a null.  We use to_chars.
	The final space is already there, from
	checksumming, and to_chars doesn't modify it.

	This is a fast way to do:

	sprintf(header->header.chksum, "%6o", sum);  */
	sprintf(h->header.chksum, "%6o", sum);
}

int write_file(int fd, const char *name, const char *data)
{
	if (!fd || !name || !data) {
		return -1;
	}
	// Write file header.
	memset(&g_buffer, 0, BLOCKSIZE);
	sprintf(g_buffer.header.name, "%s", name);
	sprintf(g_buffer.header.mode, "0000644");
	sprintf(g_buffer.header.uid, "0001000");
	sprintf(g_buffer.header.gid, "0001000");
	sprintf(g_buffer.header.magic, "ustar");
	sprintf(g_buffer.header.uname, "");
	sprintf(g_buffer.header.gname, "");
	sprintf(g_buffer.header.size, "%11o", strlen(data));
	g_buffer.header.typeflag = '0'; // Regular file.
	simple_finish_header(&g_buffer);

	int ret = 0;
	if ((ret = write(fd, g_buffer.buffer, BLOCKSIZE)) < 0) {
		return ret;
	}

	// Write data.
	memset(g_buffer.buffer, 0, BLOCKSIZE);
	strcpy(g_buffer.buffer, data);
	return write(fd, g_buffer.buffer, BLOCKSIZE);
}

int write_directory(int fd, const char *name)
{
	if (!fd || !name) {
		return -1;
	}
	// Write directory header.
	memset(g_buffer.buffer, 0, BLOCKSIZE);
	sprintf(g_buffer.header.name, "%s", name);
	sprintf(g_buffer.header.mode, "0000644");
	sprintf(g_buffer.header.uid, "0001000");
	sprintf(g_buffer.header.gid, "0001000");
	sprintf(g_buffer.header.magic, "ustar");
	sprintf(g_buffer.header.uname, "");
	sprintf(g_buffer.header.gname, "");
	sprintf(g_buffer.header.size, "%11o", 0);
	g_buffer.header.typeflag = '5'; // Directory.
	simple_finish_header(&g_buffer);

	return write(fd, g_buffer.buffer, BLOCKSIZE);
}

int write_end(int fd)
{
	int ret = 0;
	memset(&g_buffer, 0, BLOCKSIZE);
	if ((ret = write(fd, &g_buffer, BLOCKSIZE)) < 0) {
		return ret;
	}
	if ((ret = write(fd, &g_buffer, BLOCKSIZE)) < 0) {
		return ret;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int fd = 0;
	int ret = 0;

	fd = open("example.tar", O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		fprintf(stderr, "failed to open: %s\n", strerror(fd));
		return 1;
	}

	// Write file "1.txt".
	if ((ret = write_file(fd, "1.txt", "Hello World!")) < 0) {
		goto cleanup;
	}
	// Write directory "test/".
	if ((ret = write_directory(fd, "test/")) < 0) {
		goto cleanup;
	}
	// Write file "test/2.txt".
	if ((ret = write_file(fd, "test/2.txt", "Hello World Again!")) < 0) {
		goto cleanup;
	}
	// Write end blocks.
	if ((ret = write_end(fd)) < 0) {
		goto cleanup;
	}

cleanup:
	close(fd);
	if (ret < 0) {
		fprintf(stderr, "failed to write data: %d\n%s",
			ret, strerror(ret));
		return 1;
	}
	return 0;
}