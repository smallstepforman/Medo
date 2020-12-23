/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Image utililty
 */
 
#ifndef _IMAGE_UTILITY_H_
#define _IMAGE_UTILITY_H_

class BBitmap;

BBitmap *CreateThumbnail(const BBitmap *source, const float width, const float height, BBitmap *dest = nullptr);

void	PrintErrorCode(status_t code);

#endif	//#ifndef _IMAGE_UTILITY_H_
