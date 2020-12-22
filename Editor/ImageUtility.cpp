/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016
 *	DESCRIPTION:	Image utililty
 */
 
#include <cassert>
#include <cstdio>

#include <interface/Bitmap.h>
#include <support/Errors.h>

#include "ImageUtility.h"

/*	FUNCTION:		CreateThumbnail
	ARGS:			source
					width, height
					dest
	RETURN:			thumbnail, caller acquires ownership
	DESCRIPTION:	Create thumnail (RGBA32) from source image
*/
BBitmap *CreateThumbnail(const BBitmap *source, const float width, const float height, BBitmap *dest)
{
	assert(source != nullptr);
	assert((width > 0.0f) && (width <= 16384.0f));
	assert((height > 0.0f) && (height <= 16384.0f));
		
	BBitmap *image = dest ? dest : new BBitmap(BRect(0, 0, width-1, height-1), B_RGBA32);
	
	image->Lock();
	uint8 *s = (uint8 *)source->Bits();
	int32 bytes_per_row = source->BytesPerRow();
	int32 sw = bytes_per_row / 4;
	int32 sh = source->BitsLength() / bytes_per_row;
	
	float dx = sw/width;
	float dy = sh/height;
	
	uint8 *d = (uint8 *)image->Bits();
	for (int32 row=0; row < height; row++)
	{
		int32 y = dy*row;

		for (int32 col=0; col < width; col++)
		{
			int32 x= dx*col;
			*((uint32 *)d) = *((uint32 *)(s + y*bytes_per_row + x*4));
			d += 4;
		}	
	}
	image->Unlock();

	return image;
}

/*	FUNCTION:		PrintErrorCode
	ARGS:			code
	RETURN:			n/a
	DESCRIPTION:	Print error code
*/
void PrintErrorCode(status_t code)
{
	switch(code)
	{
		case B_NO_MEMORY:			printf("B_NO_MEMORY\n");			break;
		case B_IO_ERROR:			printf("B_IO_ERROR\n");				break;
		case B_PERMISSION_DENIED:	printf("B_PERMISSION_DENIED\n");	break;
		case B_BAD_INDEX:			printf("B_BAD_INDEX\n");			break;
		case B_BAD_TYPE:			printf("B_BAD_TYPE\n");				break;
		case B_BAD_VALUE:			printf("B_BAD_VALUE\n");			break;
		case B_MISMATCHED_VALUES:	printf("B_MISMATCHED_VALUES\n");	break;
		case B_NAME_NOT_FOUND:		printf("B_NAME_NOT_FOUND\n");		break;
		case B_NAME_IN_USE:			printf("B_NAME_IN_USE\n");			break;
		case B_TIMED_OUT:			printf("B_TIMED_OUT\n");			break;
		case B_INTERRUPTED:			printf("B_INTERRUPTED\n");			break;
		case B_WOULD_BLOCK:			printf("B_WOULD_BLOCK\n");			break;
		case B_CANCELED:			printf("B_CANCELED\n");				break;
		case B_NO_INIT:				printf("B_NO_INIT\n");				break;
		//case B_NOT_INITIALIZED:	printf("B_NOT_INITIALIZED\n");		break;
		case B_BUSY:				printf("B_BUSY\n");					break;
		case B_NOT_ALLOWED:			printf("B_NOT_ALLOWED\n");			break;
		case B_BAD_DATA:			printf("B_BAD_DATA\n");				break;
		case B_DONT_DO_THAT:		printf("B_DONT_DO_THAT\n");			break;
		default:					printf("Unknown (%d)\n", code);		break;
	}					
}	

