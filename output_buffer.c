#include "all.h"

OutputBuffer
OutputBuffer_Create(usize capacity)
{
	return (OutputBuffer){
	        .p = calloc(capacity, 1), .length = 0, .capacity = capacity};
}

void
OutputBuffer_Clear(OutputBuffer *b)
{
	b->length = 0;
}

void
OutputBuffer_Push(OutputBuffer *b, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	usize remaining = b->capacity - b->length;
	usize bytes_written = vsnprintf(b->p + b->length, remaining, fmt, ap);
	b->length += bytes_written;
	va_end(ap);
}

void
OutputBuffer_PushBytes(OutputBuffer *b, const char *bytes, usize count)
{
	assert(b->length + count < b->capacity);
	memcpy(b->p + b->length, bytes, count);
	b->length += count;
}
