/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Quaternion math support
*/

#ifndef _YARRA_QUATERNION_H_
#define _YARRA_QUATERNION_H_

namespace ymath
{

class YMatrix4;

//================================
class YQuaternion
{
private:
	float		x;
	float		y;
	float		z;
	float		w;

public:
	//	Constructors
	YQuaternion();
	YQuaternion(const float _x, const float _y, const float _z, const float _w)
		: x(_x), y(_y), z(_z), w(_w) {}
	YQuaternion(const float angle, const YVector3 &axis);
	YQuaternion(const YVector3 &euler) { SetFromEuler(euler); }
	YQuaternion(const YMatrix4 &matrix);

//	Setters
	void			SetZero();
	void			SetFromAngleAxis(const float angle, const YVector3 &axis);
	void			SetFromEuler(const YVector3 &euler);
	inline void		SetFromEuler(const float x_roll, const float y_pitch, const float z_yaw)
	{
		SetFromEuler(YVector3(x_roll, y_pitch, z_yaw));
	}
	void			SetFromDirectionVector(const YVector3 &direction);

	//	Conversion 
	void			GetAngleAxis(float *angle, YVector3 *axis) const;
	void			GetMatrix(YMatrix4 *matrix) const;
	const YVector3	GetEulerRotation() const;

	//	Operators
	inline const YQuaternion	operator + (const YQuaternion &q) const { return YQuaternion(x + q.x, y + q.y, z + q.z, w + q.w); }
	inline const YQuaternion	operator += (const YQuaternion &q) { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
	inline const YQuaternion	operator -= (const YQuaternion &q) { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
	const YQuaternion 			operator * (const YQuaternion &q) const;
	inline const YQuaternion	operator * (const float scaler) const { return YQuaternion(scaler*x, scaler*y, scaler*z, scaler*w); }
	inline const YQuaternion &	operator *= (const float scaler) { x *= scaler; y *= scaler; z *= scaler; w *= scaler; return *this; }
	inline const YQuaternion	operator *= (const YQuaternion &q) { return (*this = q * (*this)); }
	//	Functions
	const float					DotProduct(const YQuaternion &q) const;
	const YQuaternion &			Slerp(float time, const YQuaternion &p, const YQuaternion &q);
	const YVector3				RotateVector(const YVector3 &vec) const;
	void						PrintToStream() const;

	const YVector3 GetDirectionX(void) const { return (YVector3(1.0f - 2.0f*(y*y + z*z), 2.0f*(x*y + w*z), 2.0f*(x*z - w*y))); }
	const YVector3 GetDirectionY(void) const { return (YVector3(2.0f*(x*y - w*z), 1.0f - 2.0f*(x*x + z*z), 2.0f*(y*z + w*x))); }
	const YVector3 GetDirectionZ(void) const { return (YVector3(2.0f*(x*z + w*y), 2.0f*(y*z - w*x), 1.0f - 2.0f*(x*x + y*y))); }
};

};	//	namespace ymath

#endif // #ifndef _YARRA_QUATERNION_H_
