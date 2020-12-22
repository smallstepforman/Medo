/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified YGeometryNode from Yarra engine
*/

#include <cassert>
#include "SceneNode.h"

namespace yrender
{
	
static GLint		sMaxNumberVertexAttribs = -1;

/*****************************************************
				YGeometryNode
******************************************************/

/*	FUNCTION:		YGeometryNode :: YGeometryNode
	ARGUMENTS:		mode				GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_LINES, etc.
					buffer_format		data layout
					vertices			pointer to data
					vertices_count		glDrawArrays argument
					first				glDrawArrays parameter
					usage				glBufferData parameter		
	RETURN:			n/a
	DESCRIPTION:	Constructor.  Rendering uses glDrawArrays()
*/
YGeometryNode :: YGeometryNode(const GLenum mode, const Y_GEOMETRY_FORMAT buffer_format,
							const float *vertices, const GLsizei vertices_count,
							const GLint first, const GLenum usage)
{
	if (sMaxNumberVertexAttribs == -1)
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &sMaxNumberVertexAttribs);
	
	fVerticesBufferFormat = buffer_format;
	fIndicesBufferFormat = 0;
	fIndicesBuffer = 0;

	fDrawingMode = mode;
	fFirst = first;
	fCount = vertices_count;

	glGenVertexArrays(1, &fVertexArrayObject);
	glBindVertexArray(fVertexArrayObject);
		
	glGenBuffers(1, &fVerticesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, fVerticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices_count * yarra::yGeometryBufferSize[buffer_format], vertices, usage);
	PrepareRender();
}

/*	FUNCTION:		YGeometryNode :: YGeometryNode
	ARGUMENTS:		mode				GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_LINES, etc.
					buffer_format		layout of data
					vertices			pointer to data
					vertices_count		number of unique vertices
					indices				pointer to index data
					indices_format		GL_UNSIGNED_BYTE or GL_UNSIGNED_SHORT
					indices_count		number of vertices to draw (total)
					usage				glBufferData parameter		
	RETURN:			n/a
	DESCRIPTION:	Constructor.  Rendering uses glDrawElements()
*/
YGeometryNode :: YGeometryNode(const GLenum mode, const Y_GEOMETRY_FORMAT buffer_format,
							const float *vertices, const GLsizei vertices_count,
							const void *indices, const GLenum indices_format, const GLsizei indices_count,
							const GLenum usage)
{
	if (sMaxNumberVertexAttribs == -1)
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &sMaxNumberVertexAttribs);
	
	fVerticesBufferFormat = buffer_format;
	fIndicesBufferFormat = indices_format;

	fDrawingMode = mode;
	fFirst = 0;		// ignored for glDrawElements
	fCount = indices_count;

	glGenVertexArrays(1, &fVertexArrayObject);
	glBindVertexArray(fVertexArrayObject);
		
	glGenBuffers(1, &fVerticesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, fVerticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices_count * yarra::yGeometryBufferSize[buffer_format], vertices, usage);
		
	//	Indices array
	size_t size;
	switch (fIndicesBufferFormat)
	{
		case GL_UNSIGNED_BYTE:		size = sizeof(unsigned char);	break;
		case GL_UNSIGNED_SHORT:		size = sizeof(unsigned short);	break;
		default:					assert(0);						break;
	}
	glGenBuffers(1, &fIndicesBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fIndicesBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count*size, indices, usage);
	PrepareRender();
}

/*	FUNCTION:		YGeometryNode :: ~YGeometryNode
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YGeometryNode :: ~YGeometryNode()
{
	glDeleteVertexArrays(1, &fVertexArrayObject);
	glDeleteBuffers(1, &fVerticesBuffer);
	if (fIndicesBuffer)
		glDeleteBuffers(1, &fIndicesBuffer);
}

/*	FUNCTION:		YGeometryNode :: Render
	ARGUMENTS:		delta_time
	RETURN:			n/a
	DESCRIPTION:	Render object
*/
void YGeometryNode :: Render(float delta_time)
{
	glBindVertexArray(fVertexArrayObject);
	if (fIndicesBufferFormat)
	{
		glDrawElements(fDrawingMode, fCount, fIndicesBufferFormat, 0);
	}
	else
		glDrawArrays(fDrawingMode, fFirst, fCount);
}

/*	FUNCTION:		YGeometryNode :: UpdateVertices
	ARGUMENTS:		vertices
	RETURN:			n/a
	DESCRIPTION:	Update vertices buffer
*/
void YGeometryNode :: UpdateVertices(const float *vertices)
{
	glBindBuffer(GL_ARRAY_BUFFER, fVerticesBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, fCount * yarra::yGeometryBufferSize[fVerticesBufferFormat], vertices);
}

/*	FUNCTION:		YGeometryNode :: PrepareRender
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Enable vertex attribute array
*/
void YGeometryNode :: PrepareRender()
{
#define BUFFER_OFFSET(bytes)	((void *)((bytes)*sizeof(float)))

	int number_arrays = 0;
		
	//	Set vertex attrib array
	switch (fVerticesBufferFormat)
	{
		case Y_GEOMETRY_P3:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3), BUFFER_OFFSET(0));
			number_arrays = 1;
			break;

		case Y_GEOMETRY_P3C4U:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3C4U), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(YGeometry_P3C4U), BUFFER_OFFSET(3));
			number_arrays = 2;
			break;

		case Y_GEOMETRY_P3C4:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3C4), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3C4), BUFFER_OFFSET(3));
			number_arrays = 2;
			break;

		case Y_GEOMETRY_P3T2:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2), BUFFER_OFFSET(3));
			number_arrays = 2;
			break;

		case Y_GEOMETRY_P3T2C4:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2C4), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2C4), BUFFER_OFFSET(3));
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2C4), BUFFER_OFFSET(5));
			number_arrays = 3;
			break;

		case Y_GEOMETRY_P3T2C4U:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2C4U), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2C4U), BUFFER_OFFSET(3));
			glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(YGeometry_P3T2C4U), BUFFER_OFFSET(5));
			number_arrays = 3;
			break;

		case Y_GEOMETRY_P3N3:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3), BUFFER_OFFSET(3));
			number_arrays = 2;
			break;

		case Y_GEOMETRY_P3N3T2:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2), BUFFER_OFFSET(3));
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2), BUFFER_OFFSET(6));
			number_arrays = 3;
			break;

		case Y_GEOMETRY_P3N3T4:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T4), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T4), BUFFER_OFFSET(3));
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T4), BUFFER_OFFSET(6));
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T4), BUFFER_OFFSET(8));
			number_arrays = 4;
			break;

		case Y_GEOMETRY_P3T4C4U:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T4C4U), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T4C4U), BUFFER_OFFSET(3));
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T4C4U), BUFFER_OFFSET(5));
			glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(YGeometry_P3T4C4U), BUFFER_OFFSET(7));
			number_arrays = 4;
			break;

		case Y_GEOMETRY_P3N3T2W2B2:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2W2B2), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2W2B2), BUFFER_OFFSET(3));
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2W2B2), BUFFER_OFFSET(6));
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2W2B2), BUFFER_OFFSET(8));
			glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2W2B2), BUFFER_OFFSET(10));
			number_arrays = 5;
			break;

		case Y_GEOMETRY_P3T2W2B2:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2W2B2), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2W2B2), BUFFER_OFFSET(3));
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2W2B2), BUFFER_OFFSET(5));
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3T2W2B2), BUFFER_OFFSET(7));
			number_arrays = 4;
			break;

		case Y_GEOMETRY_P3N3T2TG4:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2TG4), BUFFER_OFFSET(0));
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2TG4), BUFFER_OFFSET(3));
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2TG4), BUFFER_OFFSET(6));
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(YGeometry_P3N3T2TG4), BUFFER_OFFSET(8));
			number_arrays = 4;
			break;

		default:
			assert(0);
	}

	//	Enable vertex attribute arrays
	for (int i = 0; i < number_arrays; i++)
		glEnableVertexAttribArray(i);
	for (int i=number_arrays; i < sMaxNumberVertexAttribs; i++)
		glDisableVertexAttribArray(i);
}

};	//	namespace yrender



