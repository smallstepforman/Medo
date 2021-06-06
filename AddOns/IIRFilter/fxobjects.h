// -----------------------------------------------------------------------------
//    ASPiK-Core File:  fxobjects.h
//
/**
    \file   fxobjects.h
    \author Will Pirkle
    \date   17-September-2018
    \brief  a collection of 54 objects and support structures, functions and
    		enuemrations for all projects documented in:

    		- Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
    		- see the book for detailed explanations of theory and inner
    		  operations of obejcts
    		- http://www.aspikplugins.com
    		- http://www.willpirkle.com
*/
// -----------------------------------------------------------------------------
#pragma once

#include <memory>
#include <math.h>
#include "guiconstants.h"
#include "filters.h"
#include <time.h>       /* time */

/** @file fxobjects.h
\brief constants
*/

// --- constants & enumerations
//
// ---  by placing outside the class declaration, these will also be avaialble to anything
//      that #includes this file (sometimes needed)
const double kSmallestPositiveFloatValue = 1.175494351e-38;         /* min positive value */
const double kSmallestNegativeFloatValue = -1.175494351e-38;         /* min negative value */
const double kSqrtTwo = pow(2.0, 0.5);
const double kMinFilterFrequency = 20.0;
const double kMaxFilterFrequency = 20480.0; // 10 octaves above 20 Hz
const double ARC4RANDOMMAX = 4294967295.0;  // (2^32 - 1)

#define NEGATIVE       0
#define POSITIVE       1

// ------------------------------------------------------------------ //
// --- FUNCTIONS ---------------------------------------------------- //
// ------------------------------------------------------------------ //

/**
@checkFloatUnderflow
\ingroup FX-Functions

@brief Perform underflow check; returns true if we did underflow (user may not care)

\param value - the value to check for underflow
\return true if overflowed, false otherwise
*/
inline bool checkFloatUnderflow(double& value)
{
	bool retValue = false;
	if (value > 0.0 && value < kSmallestPositiveFloatValue)
	{
		value = 0;
		retValue = true;
	}
	else if (value < 0.0 && value > kSmallestNegativeFloatValue)
	{
		value = 0;
		retValue = true;
	}
	return retValue;
}

/**
@doLinearInterpolation
\ingroup FX-Functions

@brief performs linear interpolation of x distance between two (x,y) points;
returns interpolated value

\param x1 - the x coordinate of the first point
\param x2 - the x coordinate of the second point
\param y1 - the y coordinate of the first point
\param y2 - the 2 coordinate of the second point
\param x - the interpolation location
\return the interpolated value or y1 if the x coordinates are unusable
*/
inline double doLinearInterpolation(double x1, double x2, double y1, double y2, double x)
{
	double denom = x2 - x1;
	if (denom == 0)
		return y1; // --- should not ever happen

	// --- calculate decimal position of x
	double dx = (x - x1) / (x2 - x1);

	// --- use weighted sum method of interpolating
	return dx*y2 + (1 - dx)*y1;
}

/**
@doLinearInterpolation
\ingroup FX-Functions

@brief performs linear interpolation of fractional x distance between two adjacent (x,y) points;
returns interpolated value

\param y1 - the y coordinate of the first point
\param y2 - the 2 coordinate of the second point
\param x - the interpolation location as a fractional distance between x1 and x2 (which are not needed)
\return the interpolated value or y2 if the interpolation is outside the x interval
*/
inline double doLinearInterpolation(double y1, double y2, double fractional_X)
{
	// --- check invalid condition
	if (fractional_X >= 1.0) return y2;

	// --- use weighted sum method of interpolating
	return fractional_X*y2 + (1.0 - fractional_X)*y1;
}

/**
@doLagrangeInterpolation
\ingroup FX-Functions

@brief implements n-order Lagrange Interpolation

\param x - Pointer to an array containing the x-coordinates of the input values
\param y - Pointer to an array containing the y-coordinates of the input values
\param n - the order of the interpolator, this is also the length of the x,y input arrays
\param xbar - The x-coorinates whose y-value we want to interpolate
\return the interpolated value
*/
inline double doLagrangeInterpolation(double* x, double* y, int n, double xbar)
{
	int i, j;
	double fx = 0.0;
	double l = 1.0;
	for (i = 0; i<n; i++)
	{
		l = 1.0;
		for (j = 0; j<n; j++)
		{
			if (j != i)
				l *= (xbar - x[j]) / (x[i] - x[j]);
		}
		fx += l*y[i];
	}
	return (fx);
}


/**
@boundValue
\ingroup FX-Functions

@brief  Bound a value to min and max limits

\param value - value to bound
\param minValue - lower bound limit
\param maxValue - upper bound limit
*/
inline void boundValue(double& value, double minValue, double maxValue)
{
	value = fmin(value, maxValue);
	value = fmax(value, minValue);
}

/**
@doUnipolarModulationFromMin
\ingroup FX-Functions

@brief Perform unipolar modulation from a min value up to a max value using a unipolar modulator value

\param unipolarModulatorValue - modulation value on range [0.0, +1.0]
\param minValue - lower modulation limit
\param maxValue - upper modulation limit
\return the modulated value
*/
inline double doUnipolarModulationFromMin(double unipolarModulatorValue, double minValue, double maxValue)
{
	// --- UNIPOLAR bound
	boundValue(unipolarModulatorValue, 0.0, 1.0);

	// --- modulate from minimum value upwards
	return unipolarModulatorValue*(maxValue - minValue) + minValue;
}

/**
@doUnipolarModulationFromMax
\ingroup FX-Functions

@brief Perform unipolar modulation from a max value down to a min value using a unipolar modulator value

\param unipolarModulatorValue - modulation value on range [0.0, +1.0]
\param minValue - lower modulation limit
\param maxValue - upper modulation limit
\return the modulated value
*/
inline double doUnipolarModulationFromMax(double unipolarModulatorValue, double minValue, double maxValue)
{
	// --- UNIPOLAR bound
	boundValue(unipolarModulatorValue, 0.0, 1.0);

	// --- modulate from maximum value downwards
	return maxValue - (1.0 - unipolarModulatorValue)*(maxValue - minValue);
}

/**
@doBipolarModulation
\ingroup FX-Functions

@brief Perform bipolar modulation about a center that his halfway between the min and max values

\param bipolarModulatorValue - modulation value on range [-1.0, +1.0]
\param minValue - lower modulation limit
\param maxValue - upper modulation limit
\return the modulated value
*/
inline double doBipolarModulation(double bipolarModulatorValue, double minValue, double maxValue)
{
	// --- BIPOLAR bound
	boundValue(bipolarModulatorValue, -1.0, 1.0);

	// --- calculate range and midpoint
	double halfRange = (maxValue - minValue) / 2.0;
	double midpoint = halfRange + minValue;

	return bipolarModulatorValue*(halfRange) + midpoint;
}

/**
@unipolarToBipolar
\ingroup FX-Functions

@brief calculates the bipolar [-1.0, +1.0] value FROM a unipolar [0.0, +1.0] value

\param value - value to convert
\return the bipolar value
*/
inline double unipolarToBipolar(double value)
{
	return 2.0*value - 1.0;
}

/**
@bipolarToUnipolar
\ingroup FX-Functions

@brief calculates the unipolar [0.0, +1.0] value FROM a bipolar [-1.0, +1.0] value

\param value - value to convert
\return the unipolar value
*/
inline double bipolarToUnipolar(double value)
{
	return 0.5*value + 0.5;
}

/**
@rawTo_dB
\ingroup FX-Functions

@brief calculates dB for given input

\param raw - value to convert to dB
\return the dB value
*/
inline double raw2dB(double raw)
{
	return 20.0*log10(raw);
}

/**
@dBTo_Raw
\ingroup FX-Functions

@brief converts dB to raw value

\param dB - value to convert to raw
\return the raw value
*/
inline double dB2Raw(double dB)
{
	return pow(10.0, (dB / 20.0));
}

/**
@peakGainFor_Q
\ingroup FX-Functions

@brief calculates the peak magnitude for a given Q

\param Q - the Q value
\return the peak gain (not in dB)
*/
inline double peakGainFor_Q(double Q)
{
	// --- no resonance at or below unity
	if (Q <= 0.707) return 1.0;
	return (Q*Q) / (pow((Q*Q - 0.25), 0.5));
}

/**
@dBPeakGainFor_Q
\ingroup FX-Functions

@brief calculates the peak magnitude in dB for a given Q

\param Q - the Q value
\return the peak gain in dB
*/
inline double dBPeakGainFor_Q(double Q)
{
	return raw2dB(peakGainFor_Q(Q));
}

/**
@doWhiteNoise
\ingroup FX-Functions

@brief calculates a random value between -1.0 and +1.0
\return the random value on the range [-1.0, +1.0]
*/
inline double doWhiteNoise()
{
	float noise = 0.0;

#if defined _WINDOWS || defined _WINDLL || defined (__HAIKU__)
	// fNoise is 0 -> 32767.0
	noise = (float)rand();

	// normalize and make bipolar
	noise = 2.0*(noise / 32767.0) - 1.0;
#else
	// fNoise is 0 -> ARC4RANDOMMAX
	noise = (float)arc4random();

	// normalize and make bipolar
	noise = 2.0*(noise / ARC4RANDOMMAX) - 1.0;
#endif

	return noise;
}

/**
@sgn
\ingroup FX-Functions

@brief calculates sgn( ) of input
\param xn - the input value
\return -1 if xn is negative or +1 if xn is 0 or greater
*/
inline double sgn(double xn)
{
	return (xn > 0) - (xn < 0);
}

/**
@calcWSGain
\ingroup FX-Functions

@brief calculates gain of a waveshaper
\param xn - the input value
\param saturation  - the saturation control
\param asymmetry  - the degree of asymmetry
\return gain value
*/
inline double calcWSGain(double xn, double saturation, double asymmetry)
{
	double g = ((xn >= 0.0 && asymmetry > 0.0) || (xn < 0.0 && asymmetry < 0.0)) ? saturation * (1.0 + 4.0*fabs(asymmetry)) : saturation;
	return g;
}

/**
@atanWaveShaper
\ingroup FX-Functions

@brief calculates arctangent waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline double atanWaveShaper(double xn, double saturation)
{
	return atan(saturation*xn) / atan(saturation);
}

/**
@tanhWaveShaper
\ingroup FX-Functions

@brief calculates hyptan waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline double tanhWaveShaper(double xn, double saturation)
{
	return tanh(saturation*xn) / tanh(saturation);
}

/**
@softClipWaveShaper
\ingroup FX-Functions

@brief calculates hyptan waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline double softClipWaveShaper(double xn, double saturation)
{
	// --- un-normalized soft clipper from Reiss book
	double d = sgn(xn);
	return sgn(xn)*(1.0 - exp(-fabs(saturation*xn)));
}

/**
@fuzzExp1WaveShaper
\ingroup FX-Functions

@brief calculates fuzz exp1 waveshaper
\param xn - the input value
\param saturation  - the saturation control
\return the waveshaped output value
*/
inline double fuzzExp1WaveShaper(double xn, double saturation, double asymmetry)
{
	// --- setup gain
	double wsGain = calcWSGain(xn, saturation, asymmetry);
	return sgn(xn)*(1.0 - exp(-fabs(wsGain*xn))) / (1.0 - exp(-wsGain));
}


/**
@getMagResponse
\ingroup FX-Functions

@brief returns the magnitude resonse of a 2nd order H(z) transfer function
\param theta - the angular frequency to apply
\param a0, a1, a2, b1, b2 - the transfer function coefficients
\return the magnigtude response of the transfer function at w = theta
*/
inline double getMagResponse(double theta, double a0, double a1, double a2, double b1, double b2)
{
	double magSqr = 0.0;
	double num = a1*a1 + (a0 - a2)*(a0 - a2) + 2.0*a1*(a0 + a2)*cos(theta) + 4.0*a0*a2*cos(theta)*cos(theta);
	double denom = b1*b1 + (1.0 - b2)*(1.0 - b2) + 2.0*b1*(1.0 + b2)*cos(theta) + 4.0*b2*cos(theta)*cos(theta);

	magSqr = num / denom;
	if (magSqr < 0.0)
		magSqr = 0.0;

	double mag = pow(magSqr, 0.5);

	return mag;
}

/**
\struct ComplexNumber
\ingroup FX-Objects
\brief Structure to hold a complex value.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct ComplexNumber
{
	ComplexNumber() {}
	ComplexNumber(double _real, double _imag)
	{
		real = _real;
		imag = _imag;
	}

	double real = 0.0; ///< real part
	double imag = 0.0; ///< imaginary part
};

/**
@complexMultiply
\ingroup FX-Functions

@brief returns the complex product of two complex numbers
\param c1, c2 - complex numbers to multiply
\return the complex product of c1 and c2
*/
inline ComplexNumber complexMultiply(ComplexNumber c1, ComplexNumber c2)
{
	ComplexNumber complexProduct;

	// --- real part
	complexProduct.real = (c1.real*c2.real) - (c1.imag*c2.imag);
	complexProduct.imag = (c1.real*c2.imag) + (c1.imag*c2.real);

	return complexProduct;
}

/**
@calcEdgeFrequencies
\ingroup FX-Functions

@brief calculagte low and high edge frequencies of BPF or BSF

\param fc - the center frequency of the BPF or BSF
\param Q - the Q (fc/BW) of the filter
\param f_Low - the returned low edge frequency
\param f_High - the returned high edge frequency
*/
inline void calcEdgeFrequencies(double fc, double Q, double& f_Low, double& f_High)
{
	bool arithmeticBW = true;
	double bandwidth = fc / Q;

	// --- geometric bw = sqrt[ (fLow)(fHigh) ]
	//     arithmetic bw = fHigh - fLow
	if (arithmeticBW)
	{
		f_Low = fc - bandwidth / 2.0;
		f_High = fc + bandwidth / 2.0;
	}
	else
	{
		; // TODO --- add geometric (for homework)
	}

}

/**
\enum brickwallFilter
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set brickwall filter type for functions that need it.

- enum class brickwallFilter { kBrickLPF, kBrickHPF, kBrickBPF, kBrickBSF };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class brickwallFilter { kBrickLPF, kBrickHPF, kBrickBPF, kBrickBSF };

/**
\struct BrickwallMagData
\ingroup FX-Objects
\brief
Custom structure that holds magnitude information about a brickwall filter.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct BrickwallMagData
{
	BrickwallMagData() {}

	brickwallFilter filterType = brickwallFilter::kBrickLPF;
	double* magArray = nullptr;
	unsigned int dftArrayLen = 0;
	double sampleRate = 44100.0;

	// --- for LPF, HPF fc = corner frequency
	//	   for BPF, BSF fc = center frequency
	double fc = 1000.0;			///< brickwall fc
	double Q = 0.707;			///< brickwall Q
	double f_Low = 500.00;		///< brickwall f_low
	double f_High = 1500.00;	///< brickwall f_high
	unsigned int relaxationBins = 0;	///< relaxation bins for FIR specification
	bool mirrorMag = false;		///< optionally mirror the output array
};

/**
\enum edgeTransition
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the edge direction (rising or falling) of a transition band.

- enum class edgeTransition { kFallingEdge, kRisingEdge };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class edgeTransition { kFallingEdge, kRisingEdge };

/**
\struct TransitionBandData
\ingroup FX-Objects
\brief
Custom structure that holds transition band information for FIR filter calculations.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct TransitionBandData
{
	TransitionBandData() {}

	edgeTransition edgeType = edgeTransition::kFallingEdge; ///< edge type
	unsigned int startBin = 0;	///< starting bin for transition band
	unsigned int stopBin = 0;	///< ending bin for transition band
	double slopeIncrement = 1.0; ///< transition slope
};


/**
@findEdgeTargetBin
\ingroup FX-Functions

@brief find bin for target frequency

\param testFreq - the target frequency
\param bin1Freq - the frequency of bin 1
\return the index of the target bin
*/
inline int findEdgeTargetBin(double testFreq, double bin1Freq)
{
	return (int)(testFreq / bin1Freq);
}

/**
@getTransitionBandData
\ingroup FX-Functions

@brief get bin data for a filter's transitino band (see FX book for details)

\param testFreq - the target frequency
\param bin1Freq - the frequency of bin 1
\param relax_Bins - numebr of bins to relax the filter specification
\param transitionData - the output parameter containing the data
\return true if data was filled, false otherwise
*/
inline bool getTransitionBandData(double testFreq, double bin1Freq, unsigned int relax_Bins, TransitionBandData& transitionData)
{
	double targetF1 = testFreq;// -(2.0*relax_Pct / 100.0)*testFreq;
	double targetF2 = testFreq + relax_Bins*bin1Freq;

	// --- for falling edge...
	int nF1 = findEdgeTargetBin(targetF1, bin1Freq);
	int nF2 = findEdgeTargetBin(targetF2, bin1Freq);
	int relaxBinsAbsDiff = nF2 - nF1;

	nF1 = fmax(0, nF1);
	nF2 = fmax(0, nF2);

	//if (relaxBinsAbsDiff < 0 || nF1 == 0)
	//	nF2 = nF1 + relaxBinsAbsDiff;

	int relaxBins = nF2 - nF1;

	if (relaxBins < 1)
		return false;

	// --- force even to make relax band symmetrical around edge
	//if (relaxBins % 2 != 0)
	//	relaxBins++;

	// --- for double-sided relax
	transitionData.startBin = nF1;
	transitionData.stopBin = relaxBins + nF1;

	// --- slope calc
	double run = transitionData.stopBin - transitionData.startBin;
	double rise = 1.0;
	if (transitionData.edgeType == edgeTransition::kFallingEdge)
		rise = -1.0;
	transitionData.slopeIncrement = rise / run;

	return true;
}

/**
@calculateBrickwallMagArray
\ingroup FX-Functions

@brief calculate an arra of magnitude points from brickwall specifications

\param magData - the the target magnitude data
\return true if data was filled, false otherwise
*/
inline bool calculateBrickwallMagArray(BrickwallMagData& magData)
{
	// --- calculate first half of array
	double actualLength = magData.mirrorMag ? (double)magData.dftArrayLen : (double)magData.dftArrayLen * 2.0;
	int dumpLength = magData.mirrorMag ? magData.dftArrayLen / 2 : magData.dftArrayLen;

	// --- first bin in Hz
	double bin1 = magData.sampleRate / actualLength;

	// --- zero out array; if filter not supported, this will return a bank of 0's!
	memset(&magData.magArray[0], 0, magData.dftArrayLen * sizeof(double));

	// --- preprocess for transition bands
	TransitionBandData fallingEdge;
	fallingEdge.edgeType = edgeTransition::kFallingEdge;

	TransitionBandData risingEdge;
	risingEdge.edgeType = edgeTransition::kRisingEdge;

	// --- this will populate FL and FH for BPF and BSF
	calcEdgeFrequencies(magData.fc, magData.Q, magData.f_Low, magData.f_High);

	bool relaxIt = false;
	if (magData.relaxationBins > 0)
	{
		if(magData.filterType == brickwallFilter::kBrickLPF)
			relaxIt = getTransitionBandData(magData.fc, bin1, magData.relaxationBins, fallingEdge);
		else if (magData.filterType == brickwallFilter::kBrickHPF)
			relaxIt = getTransitionBandData(magData.fc, bin1, magData.relaxationBins, risingEdge);
		else if (magData.filterType == brickwallFilter::kBrickBPF)
		{
			if(getTransitionBandData(magData.f_Low, bin1, magData.relaxationBins, risingEdge))
				relaxIt = getTransitionBandData(magData.f_High, bin1, magData.relaxationBins, fallingEdge);
		}
		else if (magData.filterType == brickwallFilter::kBrickBSF)
		{
			if(getTransitionBandData(magData.f_Low, bin1, magData.relaxationBins, fallingEdge))
				relaxIt = getTransitionBandData(magData.f_High, bin1, magData.relaxationBins, risingEdge);
		}
	}

	for (int i = 0; i < dumpLength; i++)
	{
		double eval_f = i*bin1;

		if (magData.filterType == brickwallFilter::kBrickLPF)
		{
			if (!relaxIt)
			{
				if (eval_f <= magData.fc)
					magData.magArray[i] = 1.0;
			}
			else // relax
			{
				if (i <= fallingEdge.startBin)
					magData.magArray[i] = 1.0;
				else if(i > fallingEdge.startBin && i < fallingEdge.stopBin)
					magData.magArray[i] = 1.0 + (i - fallingEdge.startBin)*fallingEdge.slopeIncrement;
			}
		}
		else if (magData.filterType == brickwallFilter::kBrickHPF)
		{
			if (!relaxIt)
			{
				if (eval_f >= magData.fc)
					magData.magArray[i] = 1.0;
			}
			else // relax
			{
				if (i >= risingEdge.stopBin)
					magData.magArray[i] = 1.0;
				else if (i > risingEdge.startBin && i < risingEdge.stopBin)
					magData.magArray[i] = 0.0 + (i - risingEdge.startBin)*risingEdge.slopeIncrement;
			}
		}
		else if (magData.filterType == brickwallFilter::kBrickBPF)
		{
			if (!relaxIt)
			{
				if (eval_f >= magData.f_Low && eval_f <= magData.f_High)
					magData.magArray[i] = 1.0;
			}
			else // --- frankie says relax
			{
				if (i >= risingEdge.stopBin && i <= fallingEdge.startBin)
					magData.magArray[i] = 1.0;
				else if (i > risingEdge.startBin && i < risingEdge.stopBin)
					magData.magArray[i] = 0.0 + (i - risingEdge.startBin)*risingEdge.slopeIncrement;
				else if (i > fallingEdge.startBin && i < fallingEdge.stopBin)
					magData.magArray[i] = 1.0 + (i - fallingEdge.startBin)*fallingEdge.slopeIncrement;

			}
		}
		else if (magData.filterType == brickwallFilter::kBrickBSF)
		{
			if (!relaxIt && eval_f >= magData.f_Low && eval_f <= magData.f_High)
				magData.magArray[i] = 0.0;
			else if(!relaxIt && eval_f < magData.f_Low || eval_f > magData.f_High)
				magData.magArray[i] = 1.0;
			else
			{
				// --- TODO Fill this in...
			}
		}
	}

	// -- make sure have legal first half...
	if (!magData.mirrorMag)
		return true;

	// --- now mirror the other half
	int index = magData.dftArrayLen / 2 - 1;
	for (unsigned int i = magData.dftArrayLen / 2; i <magData.dftArrayLen; i++)
	{
		magData.magArray[i] = magData.magArray[index--];
	}

	return true;
}

/**
\enum analogFilter
\ingroup Constants-Enums
\brief
Use this strongy typed enum to set the analog filter type for the AnalogMagData structure.

- analogFilter { kLPF1, kHPF1, kLPF2, kHPF2, kBPF2, kBSF2 };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class analogFilter { kLPF1, kHPF1, kLPF2, kHPF2, kBPF2, kBSF2 };

/**
\struct AnalogMagData
\ingroup Structures
\brief
Custom parameter structure calculating analog magnitude response arrays with calculateAnalogMagArray( ).

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct AnalogMagData
{
	AnalogMagData() {}

	analogFilter filterType = analogFilter::kLPF2;
	double* magArray = nullptr;
	unsigned int dftArrayLen = 0;
	double sampleRate = 44100.0;

	// --- for LPF, HPF fc = corner frequency
	//	   for BPF, BSF fc = center frequency
	double fc = 1000.0;
	double Q = 0.707;
	bool mirrorMag = true;
};

/**
@calculateAnalogMagArray
\ingroup FX-Functions

@brief calculate an arra of magnitude points from analog filter specifications

\param magData - the the target magnitude data
\return true if data was filled, false otherwise
*/
inline bool calculateAnalogMagArray(AnalogMagData& magData)
{
	// --- calculate first half of array
	double actualLength = magData.mirrorMag ? (double)magData.dftArrayLen : (double)magData.dftArrayLen * 2.0;
	int dumpLength = magData.mirrorMag ? magData.dftArrayLen / 2 : magData.dftArrayLen;

	double bin1 = magData.sampleRate / actualLength;// (double)magData.dftArrayLen;
	double zeta = 1.0 / (2.0*magData.Q);
	double w_c = 2.0*kPi*magData.fc;

	// --- zero out array; if filter not supported, this will return a bank of 0's!
	memset(&magData.magArray[0], 0, magData.dftArrayLen * sizeof(double));

	for (int i = 0; i < dumpLength; i++)
	{
		double eval_w = 2.0*kPi*i*bin1;
		double w_o = eval_w / w_c;

		if (magData.filterType == analogFilter::kLPF1)
		{
			double denXSq = 1.0 + (w_o*w_o);
			magData.magArray[i] = 1.0 / (pow(denXSq, 0.5));
		}
		else if (magData.filterType == analogFilter::kHPF1)
		{
			double denXSq = 1.0 + (w_o*w_o);
			magData.magArray[i] = w_o / (pow(denXSq, 0.5));
		}
		else if (magData.filterType == analogFilter::kLPF2)
		{
			double denXSq = (1.0 - (w_o*w_o))*(1.0 - (w_o*w_o)) + 4.0*zeta*zeta*w_o*w_o;
			magData.magArray[i] = 1.0 / (pow(denXSq, 0.5));
		}
		else if (magData.filterType == analogFilter::kHPF2)
		{
			double denXSq = (1.0 - (w_o*w_o))*(1.0 - (w_o*w_o)) + 4.0*zeta*zeta*w_o*w_o;
			magData.magArray[i] = (w_o*w_o) / (pow(denXSq, 0.5));
		}
		else if (magData.filterType == analogFilter::kBPF2)
		{
			double denXSq = (1.0 - (w_o*w_o))*(1.0 - (w_o*w_o)) + 4.0*zeta*zeta*w_o*w_o;
			magData.magArray[i] = 2.0*w_o*zeta / (pow(denXSq, 0.5));
		}
		else if (magData.filterType == analogFilter::kBSF2)
		{
			double numXSq = (1.0 - (w_o*w_o))*(1.0 - (w_o*w_o));
			double denXSq = (1.0 - (w_o*w_o))*(1.0 - (w_o*w_o)) + 4.0*zeta*zeta*w_o*w_o;
			magData.magArray[i] = (pow(numXSq, 0.5)) / (pow(denXSq, 0.5));
		}
	}

	// -- make sure have legal first half...
	if (!magData.mirrorMag)
		return true;

	// --- now mirror the other half
	int index = magData.dftArrayLen / 2 - 1;
	for (unsigned int i = magData.dftArrayLen / 2; i <magData.dftArrayLen; i++)
	{
		magData.magArray[i] = magData.magArray[index--];
	}

	return true;
}

/**
@freqSample
\ingroup FX-Functions

@brief calcuate the IR for an array of magnitude points using the frequency sampling method. NOTE: very old
function that is all over the internet.

\param N - Number of filter coefficients
\param A[] - Sample points of desired response [N/2]
\param symmetry - Symmetry of desired filter
\param h[] - the output array of impulse response
*/
inline void freqSample(int N, double A[], double h[], int symm)
{
	int n, k;
	double x, val, M;

	M = (N - 1.0) / 2.0;
	if (symm == POSITIVE)
	{
		if (N % 2)
		{
			for (n = 0; n<N; n++)
			{
				val = A[0];
				x = kTwoPi * (n - M) / N;
				for (k = 1; k <= M; k++)
					val += 2.0 * A[k] * cos(x*k);
				h[n] = val / N;
			}
		}
		else
		{
			for (n = 0; n<N; n++)
			{
				val = A[0];
				x = kTwoPi * (n - M) / N;
				for (k = 1; k <= (N / 2 - 1); k++)
					val += 2.0 * A[k] * cos(x*k);
				h[n] = val / N;
			}
		}
	}
	else
	{
		if (N % 2)
		{
			for (n = 0; n<N; n++)
			{
				val = 0;
				x = kTwoPi * (n - M) / N;
				for (k = 1; k <= M; k++)
					val += 2.0 * A[k] * sin(x*k);
				h[n] = val / N;
			}
		}
		else
		{
			for (n = 0; n<N; n++)
			{
				val = A[N / 2] * sin(kPi * (n - M));
				x = kTwoPi * (n - M) / N;
				for (k = 1; k <= (N / 2 - 1); k++)
					val += 2.0 * A[k] * sin(x*k);
				h[n] = val / N;
			}
		}
	}
}

/**
@getMagnitude
\ingroup FX-Functions

@brief calculates magnitude of a complex numb er

\param re - complex number real part
\param im - complex number imaginary part
\return the magnitude value
*/
inline double getMagnitude(double re, double im)
{
	return sqrt((re*re) + (im*im));
}

/**
@getPhase
\ingroup FX-Functions

@brief calculates phase of a complex numb er

\param re - complex number real part
\param im - complex number imaginary part
\return the phase value
*/
inline double getPhase(double re, double im)
{
	return atan2(im, re);
}

/**
@principalArg
\ingroup FX-Functions

@brief calculates proncipal argument of a phase value; this is the wrapped value on the range
of [-pi, +pi]

\param phaseIn - value to convert
\return the phase value on the range of [-pi, +pi]
*/
inline double principalArg(double phaseIn)
{
	if (phaseIn >= 0)
		return fmod(phaseIn + kPi, kTwoPi) - kPi;
	else
		return fmod(phaseIn + kPi, -kTwoPi) + kPi;
}

enum class interpolation {kLinear, kLagrange4};

/**
@resample
\ingroup FX-Functions

@brief function that resamples an input array of length N into an output array of
length M. You can set the interpolation type (linear or lagrange) and you can
supply an optional window array that the resampled array will be processed through.

\param input - input array
\param output - output array
\param inLength - length N of input buffer
\param outLength - length M of output buffer
\param interpType - linear or lagrange interpolation
\param scalar - output scaling value (optional)
\param outWindow - output windowing buffer (optional)
\return true if resampling was sucessful
*/
inline bool resample(double* input, double* output, uint32_t inLength, uint32_t outLength,
					 interpolation interpType = interpolation::kLinear,
					 double scalar = 1.0, double* outWindow = nullptr)
{
	if (inLength == 0 || outLength == 0) return false;
	if (!input || !output) return false;

	double x[4] = { 0.0 };
	double y[4] = { 0.0 };

	// --- inc
	double inc = (double)inLength / (double)(outLength);

	// --- first point
	if (outWindow)
		output[0] = outWindow[0] * scalar * input[0];
	else
		output[0] = scalar * input[0];

	if (interpType == interpolation::kLagrange4)
	{
		for (unsigned int i = 1; i < outLength; i++)
		{
			// --- find interpolation location
			double xInterp = i*inc;
			int x1 = (int)xInterp; // floor?
			double xbar = xInterp - x1;

			if (xInterp > 1 && x1 < outLength-2)
			{
				x[0] = x1 - 1;
				y[0] = input[(int)x[0]];

				x[1] = x1;
				y[1] = input[(int)x[1]];

				x[2] = x1 + 1;
				y[2] = input[(int)x[2]];

				x[3] = x1 + 2;
				y[3] = input[(int)x[3]];

				if (outWindow)
					output[i] = outWindow[i] * scalar * doLagrangeInterpolation(x, y, 4, xInterp);
				else
					output[i] = scalar * doLagrangeInterpolation(x, y, 4, xInterp);
			}
			else // --- linear for outer 2 end pts
			{
				int x2 = x1 + 1;
				if (x2 >= outLength)
					x2 = x1;
				double y1 = input[x1];
				double y2 = input[x2];

				if (outWindow)
					output[i] = outWindow[i] * scalar * doLinearInterpolation(y1, y2, xInterp - x1);
				else
					output[i] = scalar * doLinearInterpolation(y1, y2, xInterp - x1);
			}
		}
	}
	else // must be linear
	{
		// --- LINEAR INTERP
		for (unsigned int i = 1; i < outLength; i++)
		{
			double xInterp = i*inc;
			int x1 = (int)xInterp; // floor?
			int x2 = x1 + 1;
			if (x2 >= outLength)
				x2 = x1;
			double y1 = input[x1];
			double y2 = input[x2];

			if (outWindow)
				output[i] = outWindow[i] * scalar * doLinearInterpolation(y1, y2, xInterp - x1);
			else
				output[i] = scalar * doLinearInterpolation(y1, y2, xInterp - x1);
		}
	}

	return true;
}

// ------------------------------------------------------------------ //
// --- INTERFACES --------------------------------------------------- //
// ------------------------------------------------------------------ //

/**
\class IAudioSignalProcessor
\ingroup Interfaces
\brief
Use this interface for objects that process audio input samples to produce audio output samples. A derived class must implement the three abstract methods. The others are optional.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class IAudioSignalProcessor
{
public:
	// --- pure virtual, derived classes must implement or will not compile
	//     also means this is a pure abstract base class and is incomplete,
	//     so it can only be used as a base class
	//
	/** initialize the object with the new sample rate */
	virtual bool reset(double _sampleRate) = 0;

	/** process one sample in and out */
	virtual double processAudioSample(double xn) = 0;

	/** return true if the derived object can process a frame, false otherwise */
	virtual bool canProcessAudioFrame() = 0;

	/** set or change the sample rate; normally this is done during reset( ) but may be needed outside of initialzation */
	virtual void setSampleRate(double _sampleRate) {}

	/** switch to enable/disable the aux input */
	virtual void enableAuxInput(bool enableAuxInput) {}

	/** for processing objects with a sidechain input or other necessary aux input
	        the return value is optional and will depend on the subclassed object */
	virtual double processAuxInputAudioSample(double xn)
	{
		// --- do nothing
		return xn;
	}

	/** for processing objects with a sidechain input or other necessary aux input
	--- optional processing function
		e.g. does not make sense for some objects to implement this such as inherently mono objects like Biquad
			 BUT a processor that must use both left and right channels (ping-pong delay) would require it */
	virtual bool processAudioFrame(const float* inputFrame,		/* ptr to one frame of data: pInputFrame[0] = left, pInputFrame[1] = right, etc...*/
								   float* outputFrame,
								   uint32_t inputChannels,
								   uint32_t outputChannels)
	{
		// --- do nothing
		return false; // NOT handled
	}
};

/**
\struct SignalGenData
\ingroup Structures
\brief
This is the output structure for audio generator objects that can render up to four outputs.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
// --- structure to send output data from signal gen; you can add more outputs here
struct SignalGenData
{
	SignalGenData() {}

	double normalOutput = 0.0;			///< normal
	double invertedOutput = 0.0;		///< inverted
	double quadPhaseOutput_pos = 0.0;	///< 90 degrees out
	double quadPhaseOutput_neg = 0.0;	///< -90 degrees out
};

/**
\class IAudioSignalGenerator
\ingroup Interfaces
\brief
Use this interface for objects that render an output without an input, such as oscillators. May also be used for envelope generators whose input is a note-on or other switchable event.

Outpput I/F:
- Use SignalGenData structure for output.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class IAudioSignalGenerator
{
public:
	// --- pure virtual, derived classes must implement or will not compile
	//     also means this is a pure abstract base class and is incomplete,
	//     so it can only be used as a base class
	//
	/** Sample rate may or may not be required, but usually is */
	virtual bool reset(double _sampleRate) = 0;

	/** render the generator output */
	virtual const SignalGenData renderAudioOutput() = 0;
};



// ------------------------------------------------------------------ //
// --- OBJECTS ------------------------------------------------------ //
// ------------------------------------------------------------------ //
/*
Class Declarations :

class name : public IAudioSignalProcessor
	- IAudioSignalProcessor functions
	- member functions that may be called externally
	- mutators & accessors
	- helper functions(may be private / protected if needed)
	- protected member functions
*/

/**
\enum filterCoeff
\ingroup Constants-Enums
\brief
Use this enum to easily access coefficents inside of arrays.

- enum filterCoeff { a0, a1, a2, b1, b2, c0, d0, numCoeffs };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum filterCoeff { a0, a1, a2, b1, b2, c0, d0, numCoeffs };

/**
\enum stateReg
\ingroup Constants-Enums
\brief
Use this enum to easily access z^-1 state values inside of arrays. For some structures, fewer storage units are needed. They are divided as follows:

- Direct Forms: we will allow max of 2 for X (feedforward) and 2 for Y (feedback) data
- Transpose Forms: we will use ONLY the x_z1 and x_z2 registers for the 2 required delays
- enum stateReg { x_z1, x_z2, y_z1, y_z2, numStates };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/

// --- state array index values
//     z^-1 registers;
//        Direct Forms: we will allow max of 2 for X (feedforward) and 2 for Y (feedback) data
//        Transpose Forms: we will use ONLY the x_z1 and x_z2 registers for the 2 required delays
enum stateReg { x_z1, x_z2, y_z1, y_z2, numStates };

/**
\enum biquadAlgorithm
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the biquad calculation type

- enum class biquadAlgorithm { kDirect, kCanonical, kTransposeDirect, kTransposeCanonical }; //  4 types of biquad calculations, constants (k)

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/

// --- type of calculation (algorithm)
enum class biquadAlgorithm { kDirect, kCanonical, kTransposeDirect, kTransposeCanonical }; //  4 types of biquad calculations, constants (k)


/**
\struct BiquadParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the Biquad object. Default version defines the biquad structure used in the calculation.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct BiquadParameters
{
	BiquadParameters () {}

	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	BiquadParameters& operator=(const BiquadParameters& params)
	{
		if (this == &params)
			return *this;

		biquadCalcType = params.biquadCalcType;
		return *this;
	}

	biquadAlgorithm biquadCalcType = biquadAlgorithm::kDirect; ///< biquad structure to use
};

/**
\class Biquad
\ingroup FX-Objects
\brief
The Biquad object implements a first or second order H(z) transfer function using one of four standard structures: Direct, Canonical, Transpose Direct, Transpose Canonical.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use BiquadParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class Biquad : public IAudioSignalProcessor
{
public:
	Biquad() {}		/* C-TOR */
	~Biquad() {}	/* D-TOR */

	// --- IAudioSignalProcessor FUNCTIONS --- //
	//
	/** reset: clear out the state array (flush delays); can safely ignore sampleRate argument - we don't need/use it */
	virtual bool reset(double _sampleRate)
	{
		memset(&stateArray[0], 0, sizeof(double)*numStates);
		return true;  // handled = true
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through biquad to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn);

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return BiquadParameters custom data structure
	*/
	BiquadParameters getParameters() { return parameters ; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param BiquadParameters custom data structure
	*/
	void setParameters(const BiquadParameters& _parameters){ parameters = _parameters; }

	// --- MUTATORS & ACCESSORS --- //
	/** set the coefficient array NOTE: passing by pointer to array; allows us to use "array notation" with pointers i.e. [ ] */
	void setCoefficients(double* coeffs){
		// --- fast block memory copy:
		memcpy(&coeffArray[0], &coeffs[0], sizeof(double)*numCoeffs);
	}

	/** get the coefficient array for read/write access to the array (not used in current objects) */
	double* getCoefficients()
	{
		// --- read/write access to the array (not used)
		return &coeffArray[0];
	}

	/** get the state array for read/write access to the array (used only in direct form oscillator) */
	double* getStateArray()
	{
		// --- read/write access to the array (used only in direct form oscillator)
		return &stateArray[0];
	}

	/** get the structure G (gain) value for Harma filters; see 2nd Ed FX book */
	double getG_value() { return coeffArray[a0]; }

	/** get the structure S (storage) value for Harma filters; see 2nd Ed FX book */
	double getS_value();// { return storageComponent; }

protected:
	/** array of coefficients */
	double coeffArray[numCoeffs] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

	/** array of state (z^-1) registers */
	double stateArray[numStates] = { 0.0, 0.0, 0.0, 0.0 };

	/** type of calculation (algorithm  structure) */
	BiquadParameters parameters;

	/** for Harma loop resolution */
	double storageComponent = 0.0;
};


/**
\enum filterAlgorithm
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the filter algorithm for the AudioFilter object or any other object that requires filter definitions.

- filterAlgorithm { kLPF1P, kLPF1, kHPF1, kLPF2, kHPF2, kBPF2, kBSF2, kButterLPF2, kButterHPF2, kButterBPF2, kButterBSF2, kMMALPF2, kMMALPF2B, kLowShelf, kHiShelf, kNCQParaEQ, kCQParaEQ, kLWRLPF2, kLWRHPF2, kAPF1, kAPF2, kResonA, kResonB, kMatchLP2A, kMatchLP2B, kMatchBP2A, kMatchBP2B, kImpInvLP1, kImpInvLP2 };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class filterAlgorithm {
	kLPF1P, kLPF1, kHPF1, kLPF2, kHPF2, kBPF2, kBSF2, kButterLPF2, kButterHPF2, kButterBPF2,
	kButterBSF2, kMMALPF2, kMMALPF2B, kLowShelf, kHiShelf, kNCQParaEQ, kCQParaEQ, kLWRLPF2, kLWRHPF2,
	kAPF1, kAPF2, kResonA, kResonB, kMatchLP2A, kMatchLP2B, kMatchBP2A, kMatchBP2B,
	kImpInvLP1, kImpInvLP2
}; // --- you will add more here...


/**
\struct AudioFilterParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the AudioFilter object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct AudioFilterParameters
{
	AudioFilterParameters(){}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	AudioFilterParameters& operator=(const AudioFilterParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;
		algorithm = params.algorithm;
		fc = params.fc;
		Q = params.Q;
		boostCut_dB = params.boostCut_dB;

		return *this;
	}

	// --- individual parameters
	filterAlgorithm algorithm = filterAlgorithm::kLPF1; ///< filter algorithm
	double fc = 100.0; ///< filter cutoff or center frequency (Hz)
	double Q = 0.707; ///< filter Q
	double boostCut_dB = 0.0; ///< filter gain; note not used in all types
};

/**
\class AudioFilter
\ingroup FX-Objects
\brief
The AudioFilter object implements all filters in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use AudioFilterParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class AudioFilter : public IAudioSignalProcessor
{
public:
	AudioFilter() {}		/* C-TOR */
	~AudioFilter() {}		/* D-TOR */

	// --- IAudioSignalProcessor
	/** --- set sample rate, then update coeffs */
	virtual bool reset(double _sampleRate)
	{
		BiquadParameters bqp = biquad.getParameters();

		// --- you can try both forms - do you hear a difference?
		bqp.biquadCalcType = biquadAlgorithm::kTransposeCanonical; //<- this is the default operation
	//	bqp.biquadCalcType = biquadAlgorithm::kDirect; //<- this is the direct form that implements the biquad directly
		biquad.setParameters(bqp);

		sampleRate = _sampleRate;
		return biquad.reset(_sampleRate);
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn);

	/** --- sample rate change necessarily requires recalculation */
	virtual void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		calculateFilterCoeffs();
	}

	/** --- get parameters */
	AudioFilterParameters getParameters() { return audioFilterParameters; }

	/** --- set parameters */
	void setParameters(const AudioFilterParameters& parameters)
	{
		if (audioFilterParameters.algorithm != parameters.algorithm ||
			audioFilterParameters.boostCut_dB != parameters.boostCut_dB ||
			audioFilterParameters.fc != parameters.fc ||
			audioFilterParameters.Q != parameters.Q)
		{
			// --- save new params
			audioFilterParameters = parameters;
		}
		else
			return;

		// --- don't allow 0 or (-) values for Q
		if (audioFilterParameters.Q <= 0)
			audioFilterParameters.Q = 0.707;

		// --- update coeffs
		calculateFilterCoeffs();
	}

	/** --- helper for Harma filters (phaser) */
	double getG_value() { return biquad.getG_value(); }

	/** --- helper for Harma filters (phaser) */
	double getS_value() { return biquad.getS_value(); }

protected:
	// --- our calculator
	Biquad biquad; ///< the biquad object

	// --- array to hold coeffs (we need them too)
	double coeffArray[numCoeffs] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }; ///< our local copy of biquad coeffs

	// --- object parameters
	AudioFilterParameters audioFilterParameters; ///< parameters
	double sampleRate = 44100.0; ///< current sample rate

	/** --- function to recalculate coefficients due to a change in filter parameters */
	bool calculateFilterCoeffs();
};


/**
\struct FilterBankOutput
\ingroup FX-Objects
\brief
Custom output structure for filter bank objects that split the inptu into multiple frequency channels (bands)

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
// --- output for filter bank requires multiple channels (bands)
struct FilterBankOutput
{
	FilterBankOutput() {}

	// --- band-split filter output
	double LFOut = 0.0; ///< low frequency output sample
	double HFOut = 0.0;	///< high frequency output sample

	// --- add more filter channels here; or use an array[]
};


/**
\struct LRFilterBankParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the LRFilterBank object which splits the input signal into multiple bands.
The stock obejct splits into low and high frequency bands so this structure only requires one split point - add more
split frequencies to support more bands.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct LRFilterBankParameters
{
	LRFilterBankParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	LRFilterBankParameters& operator=(const LRFilterBankParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;
		splitFrequency = params.splitFrequency;
		return *this;
	}

	// --- individual parameters
	double splitFrequency = 1000.0; ///< LF/HF split frequency
};


/**
\class LRFilterBank
\ingroup FX-Objects
\brief
The LRFilterBank object implements 2 Linkwitz-Riley Filters in a parallel filter bank to split the signal into two frequency bands.
Note that one channel is inverted (see the FX book below for explanation). You can add more bands here as well.

Audio I/O:
- Processes mono input into a custom FilterBankOutput structure.
NOTE: processAudioSample( ) is inoperable and only returns the input back.

Control I/F:
- Use LRFilterBankParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class LRFilterBank : public IAudioSignalProcessor
{
public:
	LRFilterBank()		/* C-TOR */
	{
		// --- set filters as Linkwitz-Riley 2nd order
		AudioFilterParameters params = lpFilter.getParameters();
		params.algorithm = filterAlgorithm::kLWRLPF2;
		lpFilter.setParameters(params);

		params = hpFilter.getParameters();
		params.algorithm = filterAlgorithm::kLWRHPF2;
		hpFilter.setParameters(params);
	}

	~LRFilterBank() {}	/* D-TOR */

	/** reset member objects */
	virtual bool reset(double _sampleRate)
	{
		lpFilter.reset(_sampleRate);
		hpFilter.reset(_sampleRate);
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** this does nothing for this object, see processFilterBank( ) below */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		return xn;
	}

	/** process the filter bank */
	FilterBankOutput processFilterBank(double xn)
	{
		FilterBankOutput output;

		// --- process the LPF
		output.LFOut = lpFilter.processAudioSample(xn);

		// --- invert the HP filter output so that recombination will
		//     result in the correct phase and magnitude responses
		output.HFOut = -hpFilter.processAudioSample(xn);

		return output;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return LRFilterBankParameters custom data structure
	*/
	LRFilterBankParameters getParameters()
	{
		return parameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param LRFilterBankParameters custom data structure
	*/
	void setParameters(const LRFilterBankParameters& _parameters)
	{
		// --- update structure
		parameters = _parameters;

		// --- update member objects
		AudioFilterParameters params = lpFilter.getParameters();
		params.fc = parameters.splitFrequency;
		lpFilter.setParameters(params);

		params = hpFilter.getParameters();
		params.fc = parameters.splitFrequency;
		hpFilter.setParameters(params);
	}

protected:
	AudioFilter lpFilter; ///< low-band filter
	AudioFilter hpFilter; ///< high-band filter

	// --- object parameters
	LRFilterBankParameters parameters; ///< parameters for the object
};

// --- constants
const unsigned int TLD_AUDIO_DETECT_MODE_PEAK = 0;
const unsigned int TLD_AUDIO_DETECT_MODE_MS = 1;
const unsigned int TLD_AUDIO_DETECT_MODE_RMS = 2;
const double TLD_AUDIO_ENVELOPE_ANALOG_TC = -0.99967234081320612357829304641019; // ln(36.7%)

/**
\struct AudioDetectorParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the AudioDetector object. NOTE: this object uses constant defintions:

- const unsigned int TLD_AUDIO_DETECT_MODE_PEAK = 0;
- const unsigned int TLD_AUDIO_DETECT_MODE_MS = 1;
- const unsigned int TLD_AUDIO_DETECT_MODE_RMS = 2;
- const double TLD_AUDIO_ENVELOPE_ANALOG_TC = -0.99967234081320612357829304641019; // ln(36.7%)

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct AudioDetectorParameters
{
	AudioDetectorParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	AudioDetectorParameters& operator=(const AudioDetectorParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;
		attackTime_mSec = params.attackTime_mSec;
		releaseTime_mSec = params.releaseTime_mSec;
		detectMode = params.detectMode;
		detect_dB = params.detect_dB;
		clampToUnityMax = params.clampToUnityMax;
		return *this;
	}

	// --- individual parameters
	double attackTime_mSec = 0.0; ///< attack time in milliseconds
	double releaseTime_mSec = 0.0;///< release time in milliseconds
	unsigned int  detectMode = 0;///< detect mode, see TLD_ constants above
	bool detect_dB = false;	///< detect in dB  DEFAULT  = false (linear NOT log)
	bool clampToUnityMax = true;///< clamp output to 1.0 (set false for true log detectors)
};

/**
\class AudioDetector
\ingroup FX-Objects
\brief
The AudioDetector object implements the audio detector defined in the book source below.
NOTE: this detector can receive signals and transmit detection values that are both > 0dBFS

Audio I/O:
- Processes mono input to a detected signal output.

Control I/F:
- Use AudioDetectorParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class AudioDetector : public IAudioSignalProcessor
{
public:
	AudioDetector() {}	/* C-TOR */
	~AudioDetector() {}	/* D-TOR */

public:
	/** set sample rate dependent time constants and clear last envelope output value */
	virtual bool reset(double _sampleRate)
	{
		setSampleRate(_sampleRate);
		lastEnvelope = 0.0;
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	// --- process audio: detect the log envelope and return it in dB
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- all modes do Full Wave Rectification
		double input = fabs(xn);

		// --- square it for MS and RMS
		if (audioDetectorParameters.detectMode == TLD_AUDIO_DETECT_MODE_MS ||
			audioDetectorParameters.detectMode == TLD_AUDIO_DETECT_MODE_RMS)
			input *= input;

		// --- to store current
		double currEnvelope = 0.0;

		// --- do the detection with attack or release applied
		if (input > lastEnvelope)
			currEnvelope = attackTime * (lastEnvelope - input) + input;
		else
			currEnvelope = releaseTime * (lastEnvelope - input) + input;

		// --- we are recursive so need to check underflow
		checkFloatUnderflow(currEnvelope);

		// --- bound them; can happen when using pre-detector gains of more than 1.0
		if (audioDetectorParameters.clampToUnityMax)
			currEnvelope = fmin(currEnvelope, 1.0);

		// --- can not be (-)
		currEnvelope = fmax(currEnvelope, 0.0);

		// --- store envelope prior to sqrt for RMS version
		lastEnvelope = currEnvelope;

		// --- if RMS, do the SQRT
		if (audioDetectorParameters.detectMode == TLD_AUDIO_DETECT_MODE_RMS)
			currEnvelope = pow(currEnvelope, 0.5);

		// --- if not dB, we are done
		if (!audioDetectorParameters.detect_dB)
			return currEnvelope;

		// --- setup for log( )
		if (currEnvelope <= 0)
		{
			return -96.0;
		}

		// --- true log output in dB, can go above 0dBFS!
		return 20.0*log10(currEnvelope);
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return AudioDetectorParameters custom data structure
	*/
	AudioDetectorParameters getParameters()
	{
		return audioDetectorParameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param AudioDetectorParameters custom data structure
	*/
	void setParameters(const AudioDetectorParameters& parameters)
	{
		audioDetectorParameters = parameters;

		// --- update structure
		setAttackTime(audioDetectorParameters.attackTime_mSec, true);
		setReleaseTime(audioDetectorParameters.releaseTime_mSec, true);

	}

	/** set sample rate - our time constants depend on it */
	virtual void setSampleRate(double _sampleRate)
	{
		if (sampleRate == _sampleRate)
			return;

		sampleRate = _sampleRate;

		// --- recalculate RC time-constants
		setAttackTime(audioDetectorParameters.attackTime_mSec, true);
		setReleaseTime(audioDetectorParameters.releaseTime_mSec, true);
	}

protected:
	AudioDetectorParameters audioDetectorParameters; ///< parameters for object
	double attackTime = 0.0;	///< attack time coefficient
	double releaseTime = 0.0;	///< release time coefficient
	double sampleRate = 44100;	///< stored sample rate
	double lastEnvelope = 0.0;	///< output register

	/** set our internal atack time coefficients based on times and sample rate */
	void setAttackTime(double attack_in_ms, bool forceCalc = false);

	/** set our internal release time coefficients based on times and sample rate */
	void setReleaseTime(double release_in_ms, bool forceCalc = false);
};


/**
\enum dynamicsProcessorType
\ingroup Constants-Enums
\brief
Use this strongly typed enum to set the dynamics processor type.

- enum class dynamicsProcessorType { kCompressor, kDownwardExpander };
- limiting is the extreme version of kCompressor
- gating is the extreme version of kDownwardExpander

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/

// --- processorType
enum class dynamicsProcessorType { kCompressor, kDownwardExpander };


/**
\struct DynamicsProcessorParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the DynamicsProcessor object. Ths struct includes all information needed from GUI controls.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct DynamicsProcessorParameters
{
	DynamicsProcessorParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	DynamicsProcessorParameters& operator=(const DynamicsProcessorParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		ratio = params.ratio;
		threshold_dB = params.threshold_dB;
		kneeWidth_dB = params.kneeWidth_dB;
		hardLimitGate = params.hardLimitGate;
		softKnee = params.softKnee;
		enableSidechain = params.enableSidechain;
		calculation = params.calculation;
		attackTime_mSec = params.attackTime_mSec;
		releaseTime_mSec = params.releaseTime_mSec;
		outputGain_dB = params.outputGain_dB;
		// --- NOTE: do not set outbound variables??
		gainReduction = params.gainReduction;
		gainReduction_dB = params.gainReduction_dB;
		return *this;
	}

	// --- individual parameters
	double ratio = 50.0;				///< processor I/O gain ratio
	double threshold_dB = -10.0;		///< threshold in dB
	double kneeWidth_dB = 10.0;			///< knee width in dB for soft-knee operation
	bool hardLimitGate = false;			///< threshold in dB
	bool softKnee = true;				///< soft knee flag
	bool enableSidechain = false;		///< enable external sidechain input to object
	dynamicsProcessorType calculation = dynamicsProcessorType::kCompressor; ///< processor calculation type
	double attackTime_mSec = 0.0;		///< attack mSec
	double releaseTime_mSec = 0.0;		///< release mSec
	double outputGain_dB = 0.0;			///< make up gain

	// --- outbound values, for owner to use gain-reduction metering
	double gainReduction = 1.0;			///< output value for gain reduction that occurred
	double gainReduction_dB = 0.0;		///< output value for gain reduction that occurred in dB
};

/**
\class DynamicsProcessor
\ingroup FX-Objects
\brief
The DynamicsProcessor object implements a dynamics processor suite: compressor, limiter, downward expander, gate.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use DynamicsProcessorParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class DynamicsProcessor : public IAudioSignalProcessor
{
public:
	DynamicsProcessor() {}	/* C-TOR */
	~DynamicsProcessor() {}	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sidechainInputSample = 0.0;
		detector.reset(_sampleRate);
		AudioDetectorParameters detectorParams = detector.getParameters();
		detectorParams.clampToUnityMax = false;
		detectorParams.detect_dB = true;
		detector.setParameters(detectorParams);
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** enable sidechaib input */
	virtual void enableAuxInput(bool enableAuxInput){ parameters.enableSidechain = enableAuxInput; }

	/** process the sidechain by saving the value for the upcoming processAudioSample() call */
	virtual double processAuxInputAudioSample(double xn)
	{
		sidechainInputSample = xn;
		return sidechainInputSample;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return DynamicsProcessorParameters custom data structure
	*/
	DynamicsProcessorParameters getParameters(){ return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param DynamicsProcessorParameters custom data structure
	*/
	void setParameters(const DynamicsProcessorParameters& _parameters)
	{
		parameters = _parameters;

		AudioDetectorParameters detectorParams = detector.getParameters();
		detectorParams.attackTime_mSec = parameters.attackTime_mSec;
		detectorParams.releaseTime_mSec = parameters.releaseTime_mSec;
		detector.setParameters(detectorParams);
	}

	/** process audio using feed-forward dynamics processor flowchart */
	/*
		1. detect input signal
		2. calculate gain
		3. apply to input sample
	*/
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- detect input
		double detect_dB = 0.0;

		// --- if using the sidechain, process the aux input
		if(parameters.enableSidechain)
			detect_dB = detector.processAudioSample(sidechainInputSample);
		else
			detect_dB = detector.processAudioSample(xn);

		// --- compute gain
		double gr = computeGain(detect_dB);

		// --- makeup gain
		double makeupGain = pow(10.0, parameters.outputGain_dB / 20.0);

		// --- do DCA + makeup gain
		return xn * gr * makeupGain;
	}

protected:
	DynamicsProcessorParameters parameters; ///< object parameters
	AudioDetector detector; ///< the sidechain audio detector

	// --- storage for sidechain audio input (mono only)
	double sidechainInputSample = 0.0; ///< storage for sidechain sample

	/** compute (and save) the current gain value based on detected input (dB) */
	inline double computeGain(double detect_dB)
	{
		double output_dB = 0.0;

		if (parameters.calculation == dynamicsProcessorType::kCompressor)
		{
			// --- hard knee
			if (!parameters.softKnee)
			{
				// --- below threshold, unity
				if (detect_dB <= parameters.threshold_dB)
					output_dB = detect_dB;
				else// --- above threshold, compress
				{
					if (parameters.hardLimitGate) // is limiter?
						output_dB = parameters.threshold_dB;
					else
						output_dB = parameters.threshold_dB + (detect_dB - parameters.threshold_dB) / parameters.ratio;
				}
			}
			else // --- calc gain with knee
			{
				// --- left side of knee, outside of width, unity gain zone
				if (2.0*(detect_dB - parameters.threshold_dB) < -parameters.kneeWidth_dB)
					output_dB = detect_dB;
				// --- else inside the knee,
				else if (2.0*(fabs(detect_dB - parameters.threshold_dB)) <= parameters.kneeWidth_dB)
				{
					if (parameters.hardLimitGate)	// --- is limiter?
						output_dB = detect_dB - pow((detect_dB - parameters.threshold_dB + (parameters.kneeWidth_dB / 2.0)), 2.0) / (2.0*parameters.kneeWidth_dB);
					else // --- 2nd order poly
						output_dB = detect_dB + (((1.0 / parameters.ratio) - 1.0) * pow((detect_dB - parameters.threshold_dB + (parameters.kneeWidth_dB / 2.0)), 2.0)) / (2.0*parameters.kneeWidth_dB);
				}
				// --- right of knee, compression zone
				else if (2.0*(detect_dB - parameters.threshold_dB) > parameters.kneeWidth_dB)
				{
					if (parameters.hardLimitGate) // --- is limiter?
						output_dB = parameters.threshold_dB;
					else
						output_dB = parameters.threshold_dB + (detect_dB - parameters.threshold_dB) / parameters.ratio;
				}
			}
		}
		else if (parameters.calculation == dynamicsProcessorType::kDownwardExpander)
		{
			// --- hard knee
			// --- NOTE: soft knee is not technically possible with a gate because there
			//           is no "left side" of the knee
			if (!parameters.softKnee || parameters.hardLimitGate)
			{
				// --- above threshold, unity gain
				if (detect_dB >= parameters.threshold_dB)
					output_dB = detect_dB;
				else
				{
					if (parameters.hardLimitGate) // --- gate: -inf(dB)
						output_dB = -1.0e34;
					else
						output_dB = parameters.threshold_dB + (detect_dB - parameters.threshold_dB) * parameters.ratio;
				}
			}
			else // --- calc gain with knee
			{
				// --- right side of knee, unity gain zone
				if (2.0*(detect_dB - parameters.threshold_dB) > parameters.kneeWidth_dB)
					output_dB = detect_dB;
				// --- in the knee
				else if (2.0*(fabs(detect_dB - parameters.threshold_dB)) > -parameters.kneeWidth_dB)
					output_dB = ((parameters.ratio - 1.0) * pow((detect_dB - parameters.threshold_dB - (parameters.kneeWidth_dB / 2.0)), 2.0)) / (2.0*parameters.kneeWidth_dB);
				// --- left side of knee, downward expander zone
				else if (2.0*(detect_dB - parameters.threshold_dB) <= -parameters.kneeWidth_dB)
					output_dB = parameters.threshold_dB + (detect_dB - parameters.threshold_dB) * parameters.ratio;
			}
		}

		// --- convert gain; store values for user meters
		parameters.gainReduction_dB = output_dB - detect_dB;
		parameters.gainReduction = pow(10.0, (parameters.gainReduction_dB) / 20.0);

		// --- the current gain coefficient value
		return parameters.gainReduction;
	}
};

/**
\class LinearBuffer
\ingroup FX-Objects
\brief
The LinearBuffer object implements a linear buffer of type T. It allows easy wrapping of a smart pointer object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
template <typename T>
class LinearBuffer
{
public:
	LinearBuffer() {}	/* C-TOR */
	~LinearBuffer() {}	/* D-TOR */

	/** flush buffer by resetting all values to 0.0 */
	void flushBuffer() { memset(&buffer[0], 0, bufferLength * sizeof(T)); }

	/** Create a buffer based on a target maximum in SAMPLES
	//	   do NOT call from realtime audio thread; do this prior to any processing */
	void createLinearBuffer(unsigned int _bufferLength)
	{
		// --- find nearest power of 2 for buffer, save it as bufferLength
		bufferLength = _bufferLength;

		// --- create new buffer
		buffer.reset(new T[bufferLength]);

		// --- flush buffer
		flushBuffer();
	}

	/** write a value into the buffer; this overwrites the previous oldest value in the buffer */
	void writeBuffer(unsigned int index, T input)
	{
		if (index >= bufferLength) return;

		// --- write and increment index counter
		buffer[index] = input;
	}

	/**  read an arbitrary location that is delayInSamples old */
	T readBuffer(unsigned int index)//, bool readBeforeWrite = true)
	{
		if (index >= bufferLength) return 0.0;

		// --- read it
		return buffer[index];
	}

private:
	std::unique_ptr<T[]> buffer = nullptr;	///< smart pointer will auto-delete
	unsigned int bufferLength = 1024; ///< buffer length
};


/**
\class CircularBuffer
\ingroup FX-Objects
\brief
The CircularBuffer object implements a simple circular buffer. It uses a wrap mask to wrap the read or write index quickly.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
/** A simple cyclic buffer: NOTE - this is NOT an IAudioSignalProcessor or IAudioSignalGenerator
	S must be a power of 2.
*/
template <typename T>
class CircularBuffer
{
public:
	CircularBuffer() {}		/* C-TOR */
	~CircularBuffer() {}	/* D-TOR */

							/** flush buffer by resetting all values to 0.0 */
	void flushBuffer(){ memset(&buffer[0], 0, bufferLength * sizeof(T)); }

	/** Create a buffer based on a target maximum in SAMPLES
	//	   do NOT call from realtime audio thread; do this prior to any processing */
	void createCircularBuffer(unsigned int _bufferLength)
	{
		// --- find nearest power of 2 for buffer, and create
		createCircularBufferPowerOfTwo((unsigned int)(pow(2, ceil(log(_bufferLength) / log(2)))));
	}

	/** Create a buffer based on a target maximum in SAMPLESwhere the size is
	    pre-calculated as a power of two */
	void createCircularBufferPowerOfTwo(unsigned int _bufferLengthPowerOfTwo)
	{
		// --- reset to top
		writeIndex = 0;

		// --- find nearest power of 2 for buffer, save it as bufferLength
		bufferLength = _bufferLengthPowerOfTwo;

		// --- save (bufferLength - 1) for use as wrapping mask
		wrapMask = bufferLength - 1;

		// --- create new buffer
		buffer.reset(new T[bufferLength]);

		// --- flush buffer
		flushBuffer();
	}

	/** write a value into the buffer; this overwrites the previous oldest value in the buffer */
	void writeBuffer(T input)
	{
		// --- write and increment index counter
		buffer[writeIndex++] = input;

		// --- wrap if index > bufferlength - 1
		writeIndex &= wrapMask;
	}

	/** read an arbitrary location that is delayInSamples old */
	T readBuffer(int delayInSamples)//, bool readBeforeWrite = true)
	{
		// --- subtract to make read index
		//     note: -1 here is because we read-before-write,
		//           so the *last* write location is what we use for the calculation
		int readIndex = (writeIndex - 1) - delayInSamples;

		// --- autowrap index
		readIndex &= wrapMask;

		// --- read it
		return buffer[readIndex];
	}

	/** read an arbitrary location that includes a fractional sample */
	T readBuffer(double delayInFractionalSamples)
	{
		// --- truncate delayInFractionalSamples and read the int part
		T y1 = readBuffer((int)delayInFractionalSamples);

		// --- if no interpolation, just return value
		if (!interpolate) return y1;

		// --- else do interpolation
		//
		// --- read the sample at n+1 (one sample OLDER)
		T y2 = readBuffer((int)delayInFractionalSamples + 1);

		// --- get fractional part
		double fraction = delayInFractionalSamples - (int)delayInFractionalSamples;

		// --- do the interpolation (you could try different types here)
		return doLinearInterpolation(y1, y2, fraction);
	}

	/** enable or disable interpolation; usually used for diagnostics or in algorithms that require strict integer samples times */
	void setInterpolate(bool b) { interpolate = b; }

private:
	std::unique_ptr<T[]> buffer = nullptr;	///< smart pointer will auto-delete
	unsigned int writeIndex = 0;		///> write index
	unsigned int bufferLength = 1024;	///< must be nearest power of 2
	unsigned int wrapMask = 1023;		///< must be (bufferLength - 1)
	bool interpolate = true;			///< interpolation (default is ON)
};


/**
\class ImpulseConvolver
\ingroup FX-Objects
\brief
The ImpulseConvolver object implements a linear conovlver. NOTE: compile in Release mode or you may experice stuttering,
glitching or other sample-drop activity.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- none.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class ImpulseConvolver : public IAudioSignalProcessor
{
public:
	ImpulseConvolver() {
		init(512);
	}		/* C-TOR */
	~ImpulseConvolver() {}		/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- flush signal buffer; IR buffer is static
		signalBuffer.flushBuffer();
		return true;
	}

	/** process one input - note this is CPU intensive as it performs simple linear convolution */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		double output = 0.0;

		// --- write buffer; x(n) overwrites oldest value
		//     this is the only time we do not read before write!
		signalBuffer.writeBuffer(xn);

		// --- do the convolution
		for (unsigned int i = 0; i < length; i++)
		{
			// --- y(n) += x(n)h(n)
			//     for signalBuffer.readBuffer(0) -> x(n)
			//		   signalBuffer.readBuffer(n-D)-> x(n-D)
			double signal = signalBuffer.readBuffer((int)i);
			double irrrrr = irBuffer.readBuffer((int)i);
			output += signal*irrrrr;

			//output += signalBuffer.readBuffer((int)i) * irBuffer.readBuffer((int)i);
		}

		return output;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** create the buffer based on the exact power of 2 */
	void init(unsigned int lengthPowerOfTwo)
	{
		length = lengthPowerOfTwo;
		// --- create (and clear out) the buffers
		signalBuffer.createCircularBufferPowerOfTwo(lengthPowerOfTwo);
		irBuffer.createLinearBuffer(lengthPowerOfTwo);
	}

	/** set the impulse response */
	void setImpulseResponse(double* irArray, unsigned int lengthPowerOfTwo)
	{
		if (lengthPowerOfTwo != length)
		{
			length = lengthPowerOfTwo;
			// --- create (and clear out) the buffers
			signalBuffer.createCircularBufferPowerOfTwo(lengthPowerOfTwo);
			irBuffer.createLinearBuffer(lengthPowerOfTwo);
		}

		// --- load up the IR buffer
		for (unsigned int i = 0; i < length; i++)
		{
			irBuffer.writeBuffer(i, irArray[i]);
		}
	}

protected:
	// --- delay buffer of doubles
	CircularBuffer<double> signalBuffer; ///< circulat buffer for the signal
	LinearBuffer<double> irBuffer;	///< linear buffer for the IR

	unsigned int length = 0;	///< length of convolution (buffer)

};

const unsigned int IR_LEN = 512;
/**
\struct AnalogFIRFilterParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the AnalogFIRFilter object. This is a somewhat silly object that implaments an analog
magnitude response as a FIR filter. NOT DESIGNED to replace virtual analog; rather it is intended to show the
frequency sampling method in an easy (and fun) way.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct AnalogFIRFilterParameters
{
	AnalogFIRFilterParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	AnalogFIRFilterParameters& operator=(const AnalogFIRFilterParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		filterType = params.filterType;
		fc = params.fc;
		Q = params.Q;

		return *this;
	}

	// --- individual parameters
	analogFilter filterType = analogFilter::kLPF1; ///< filter type
	double fc = 0.0;	///< filter fc
	double Q = 0.0;		///< filter Q
};

/**
\class AnalogFIRFilter
\ingroup FX-Objects
\brief
The AnalogFIRFilter object implements a somewhat silly algorithm that implaments an analog
magnitude response as a FIR filter. NOT DESIGNED to replace virtual analog; rather it is intended to show the
frequency sampling method in an easy (and fun) way.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use AnalogFIRFilterParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class AnalogFIRFilter : public IAudioSignalProcessor
{
public:
	AnalogFIRFilter() {}	/* C-TOR */
	~AnalogFIRFilter() {}	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		convolver.reset(_sampleRate);
		convolver.init(IR_LEN);
		return true;
	}

	/** pefrorm the convolution */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- do the linear convolution
		return convolver.processAudioSample(xn);
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return AnalogFIRFilterParameters custom data structure
	*/
	AnalogFIRFilterParameters getParameters() { return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param AnalogFIRFilterParameters custom data structure
	*/
	void setParameters(AnalogFIRFilterParameters _parameters)
	{
		if (_parameters.fc != parameters.fc ||
			_parameters.Q != parameters.Q ||
			_parameters.filterType != parameters.filterType)
		{
			// --- set the filter IR for the convolver
			AnalogMagData analogFilterData;
			analogFilterData.sampleRate = sampleRate;
			analogFilterData.magArray = &analogMagArray[0];
			analogFilterData.dftArrayLen = IR_LEN;
			analogFilterData.mirrorMag = false;

			analogFilterData.filterType = _parameters.filterType;
			analogFilterData.fc = _parameters.fc; // 1000.0;
			analogFilterData.Q = _parameters.Q;

			// --- calculate the analog mag array
			calculateAnalogMagArray(analogFilterData);

			// --- frequency sample the mag array
			freqSample(IR_LEN, analogMagArray, irArray, POSITIVE);

			// --- update new frequency response
			convolver.setImpulseResponse(irArray, IR_LEN);
		}

		parameters = _parameters;
	}

private:
	AnalogFIRFilterParameters parameters; ///< object parameters
	ImpulseConvolver convolver; ///< convolver object to perform FIR convolution
	double analogMagArray[IR_LEN] = { 0.0 }; ///< array for analog magnitude response
	double irArray[IR_LEN] = { 0.0 }; ///< array to hold calcualted IR
	double sampleRate = 0.0; ///< storage for sample rate
};

/**
\enum delayAlgorithm
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the delay algorithm

- enum class delayAlgorithm { kNormal, kPingPong };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class delayAlgorithm { kNormal, kPingPong };

/**
\enum delayUpdateType
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the delay update type; this varies depending on the designer's choice
of GUI controls. See the book reference for more details.

- enum class delayUpdateType { kLeftAndRight, kLeftPlusRatio };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class delayUpdateType { kLeftAndRight, kLeftPlusRatio };


/**
\struct AudioDelayParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the AudioDelay object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct AudioDelayParameters
{
	AudioDelayParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	AudioDelayParameters& operator=(const AudioDelayParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		algorithm = params.algorithm;
		wetLevel_dB = params.wetLevel_dB;
		dryLevel_dB = params.dryLevel_dB;
		feedback_Pct = params.feedback_Pct;

		updateType = params.updateType;
		leftDelay_mSec = params.leftDelay_mSec;
		rightDelay_mSec = params.rightDelay_mSec;
		delayRatio_Pct = params.delayRatio_Pct;

		return *this;
	}

	// --- individual parameters
	delayAlgorithm algorithm = delayAlgorithm::kNormal; ///< delay algorithm
	double wetLevel_dB = -3.0;	///< wet output level in dB
	double dryLevel_dB = -3.0;	///< dry output level in dB
	double feedback_Pct = 0.0;	///< feedback as a % value

	delayUpdateType updateType = delayUpdateType::kLeftAndRight;///< update algorithm
	double leftDelay_mSec = 0.0;	///< left delay time
	double rightDelay_mSec = 0.0;	///< right delay time
	double delayRatio_Pct = 100.0;	///< dela ratio: right length = (delayRatio)*(left length)
};

/**
\class AudioDelay
\ingroup FX-Objects
\brief
The AudioDelay object implements a stereo audio delay with multiple delay algorithms.

Audio I/O:
- Processes mono input to mono output OR stereo output.

Control I/F:
- Use AudioDelayParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class AudioDelay : public IAudioSignalProcessor
{
public:
	AudioDelay() {}		/* C-TOR */
	~AudioDelay() {}	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- if sample rate did not change
		if (sampleRate == _sampleRate)
		{
			// --- just flush buffer and return
			delayBuffer_L.flushBuffer();
			delayBuffer_R.flushBuffer();
			return true;
		}

		// --- create new buffer, will store sample rate and length(mSec)
		createDelayBuffers(_sampleRate, bufferLength_mSec);

		return true;
	}

	/** process MONO audio delay */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- read delay
		double yn = delayBuffer_L.readBuffer(delayInSamples_L);

		// --- create input for delay buffer
		double dn = xn + (parameters.feedback_Pct / 100.0) * yn;

		// --- write to delay buffer
		delayBuffer_L.writeBuffer(dn);

		// --- form mixture out = dry*xn + wet*yn
		double output = dryMix*xn + wetMix*yn;

		return output;
	}

	/** return true: this object can also process frames */
	virtual bool canProcessAudioFrame() { return true; }

	/** process STEREO audio delay in frames */
	virtual bool processAudioFrame(const float* inputFrame,		/* ptr to one frame of data: pInputFrame[0] = left, pInputFrame[1] = right, etc...*/
		float* outputFrame,
		uint32_t inputChannels,
		uint32_t outputChannels)
	{
		// --- make sure we have input and outputs
		if (inputChannels == 0 || outputChannels == 0)
			return false;

		// --- make sure we support this delay algorithm
		if (parameters.algorithm != delayAlgorithm::kNormal &&
			parameters.algorithm != delayAlgorithm::kPingPong)
			return false;

		// --- if only one output channel, revert to mono operation
		if (outputChannels == 1)
		{
			// --- process left channel only
			outputFrame[0] = processAudioSample(inputFrame[0]);
			return true;
		}

		// --- if we get here we know we have 2 output channels
		//
		// --- pick up inputs
		//
		// --- LEFT channel
		double xnL = inputFrame[0];

		// --- RIGHT channel (duplicate left input if mono-in)
		double xnR = inputChannels > 1 ? inputFrame[1] : xnL;

		// --- read delay LEFT
		double ynL = delayBuffer_L.readBuffer(delayInSamples_L);

		// --- read delay RIGHT
		double ynR = delayBuffer_R.readBuffer(delayInSamples_R);

		// --- create input for delay buffer with LEFT channel info
		double dnL = xnL + (parameters.feedback_Pct / 100.0) * ynL;

		// --- create input for delay buffer with RIGHT channel info
		double dnR = xnR + (parameters.feedback_Pct / 100.0) * ynR;

		// --- decode
		if (parameters.algorithm == delayAlgorithm::kNormal)
		{
			// --- write to LEFT delay buffer with LEFT channel info
			delayBuffer_L.writeBuffer(dnL);

			// --- write to RIGHT delay buffer with RIGHT channel info
			delayBuffer_R.writeBuffer(dnR);
		}
		else if (parameters.algorithm == delayAlgorithm::kPingPong)
		{
			// --- write to LEFT delay buffer with RIGHT channel info
			delayBuffer_L.writeBuffer(dnR);

			// --- write to RIGHT delay buffer with LEFT channel info
			delayBuffer_R.writeBuffer(dnL);
		}

		// --- form mixture out = dry*xn + wet*yn
		double outputL = dryMix*xnL + wetMix*ynL;

		// --- form mixture out = dry*xn + wet*yn
		double outputR = dryMix*xnR + wetMix*ynR;

		// --- set left channel
		outputFrame[0] = outputL;

		// --- set right channel
		outputFrame[1] = outputR;

		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return AudioDelayParameters custom data structure
	*/
	AudioDelayParameters getParameters() { return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param AudioDelayParameters custom data structure
	*/
	void setParameters(AudioDelayParameters _parameters)
	{
		// --- check mix in dB for calc
		if (_parameters.dryLevel_dB != parameters.dryLevel_dB)
			dryMix = pow(10.0, _parameters.dryLevel_dB / 20.0);
		if (_parameters.wetLevel_dB != parameters.wetLevel_dB)
			wetMix = pow(10.0, _parameters.wetLevel_dB / 20.0);

		// --- save; rest of updates are cheap on CPU
		parameters = _parameters;

		// --- check update type first:
		if (parameters.updateType == delayUpdateType::kLeftAndRight)
		{
			// --- set left and right delay times
			// --- calculate total delay time in samples + fraction
			double newDelayInSamples_L = parameters.leftDelay_mSec*(samplesPerMSec);
			double newDelayInSamples_R = parameters.rightDelay_mSec*(samplesPerMSec);

			// --- new delay time with fraction
			delayInSamples_L = newDelayInSamples_L;
			delayInSamples_R = newDelayInSamples_R;
		}
		else if (parameters.updateType == delayUpdateType::kLeftPlusRatio)
		{
			// --- get and validate ratio
			double delayRatio = parameters.delayRatio_Pct / 100.0;
			boundValue(delayRatio, 0.0, 1.0);

			// --- calculate total delay time in samples + fraction
			double newDelayInSamples = parameters.leftDelay_mSec*(samplesPerMSec);

			// --- new delay time with fraction
			delayInSamples_L = newDelayInSamples;
			delayInSamples_R = delayInSamples_L*delayRatio;
		}
	}

	/** creation function */
	void createDelayBuffers(double _sampleRate, double _bufferLength_mSec)
	{
		// --- store for math
		bufferLength_mSec = _bufferLength_mSec;
		sampleRate = _sampleRate;
		samplesPerMSec = sampleRate / 1000.0;

		// --- total buffer length including fractional part
		bufferLength = (unsigned int)(bufferLength_mSec*(samplesPerMSec)) + 1; // +1 for fractional part

																			   // --- create new buffer
		delayBuffer_L.createCircularBuffer(bufferLength);
		delayBuffer_R.createCircularBuffer(bufferLength);
	}

private:
	AudioDelayParameters parameters; ///< object parameters

	double sampleRate = 0.0;		///< current sample rate
	double samplesPerMSec = 0.0;	///< samples per millisecond, for easy access calculation
	double delayInSamples_L = 0.0;	///< double includes fractional part
	double delayInSamples_R = 0.0;	///< double includes fractional part
	double bufferLength_mSec = 0.0;	///< buffer length in mSec
	unsigned int bufferLength = 0;	///< buffer length in samples
	double wetMix = 0.707; ///< wet output default = -3dB
	double dryMix = 0.707; ///< dry output default = -3dB

	// --- delay buffer of doubles
	CircularBuffer<double> delayBuffer_L;	///< LEFT delay buffer of doubles
	CircularBuffer<double> delayBuffer_R;	///< RIGHT delay buffer of doubles
};


/**
\enum generatorWaveform
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the oscillator waveform

- enum  generatorWaveform { kTriangle, kSin, kSaw };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class generatorWaveform { kTriangle, kSin, kSaw };

/**
\struct OscillatorParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the LFO and DFOscillator objects.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct OscillatorParameters
{
	OscillatorParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	OscillatorParameters& operator=(const OscillatorParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		waveform = params.waveform;
		frequency_Hz = params.frequency_Hz;
		return *this;
	}

	// --- individual parameters
	generatorWaveform waveform = generatorWaveform::kTriangle; ///< the current waveform
	double frequency_Hz = 0.0;	///< oscillator frequency
};

/**
\class LFO
\ingroup FX-Objects
\brief
The LFO object implements a mathematically perfect LFO generator for modulation uses only. It should not be used for
audio frequencies except for the sinusoidal output which, though an approximation, has very low TDH.

Audio I/O:
- Output only object: low frequency generator.

Control I/F:
- Use OscillatorParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class LFO : public IAudioSignalGenerator
{
public:
	LFO() {	srand(time(NULL)); }	/* C-TOR */
	virtual ~LFO() {}				/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		phaseInc = lfoParameters.frequency_Hz / sampleRate;

		// --- timebase variables
		modCounter = 0.0;			///< modulo counter [0.0, +1.0]
		modCounterQP = 0.25;		///<Quad Phase modulo counter [0.0, +1.0]

		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return OscillatorParameters custom data structure
	*/
	OscillatorParameters getParameters(){ return lfoParameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param OscillatorParameters custom data structure
	*/
	void setParameters(const OscillatorParameters& params)
	{
		if(params.frequency_Hz != lfoParameters.frequency_Hz)
			// --- update phase inc based on osc freq and fs
			phaseInc = params.frequency_Hz / sampleRate;

		lfoParameters = params;
	}

	/** render a new audio output structure */
	virtual const SignalGenData renderAudioOutput();

protected:
	// --- parameters
	OscillatorParameters lfoParameters; ///< obejcgt parameters

	// --- sample rate
	double sampleRate = 0.0;			///< sample rate

	// --- timebase variables
	double modCounter = 0.0;			///< modulo counter [0.0, +1.0]
	double phaseInc = 0.0;				///< phase inc = fo/fs
	double modCounterQP = 0.25;			///<Quad Phase modulo counter [0.0, +1.0]

	/** check the modulo counter and wrap if needed */
	inline bool checkAndWrapModulo(double& moduloCounter, double phaseInc)
	{
		// --- for positive frequencies
		if (phaseInc > 0 && moduloCounter >= 1.0)
		{
			moduloCounter -= 1.0;
			return true;
		}

		// --- for negative frequencies
		if (phaseInc < 0 && moduloCounter <= 0.0)
		{
			moduloCounter += 1.0;
			return true;
		}

		return false;
	}

	/** advanvce the modulo counter, then check the modulo counter and wrap if needed */
	inline bool advanceAndCheckWrapModulo(double& moduloCounter, double phaseInc)
	{
		// --- advance counter
		moduloCounter += phaseInc;

		// --- for positive frequencies
		if (phaseInc > 0 && moduloCounter >= 1.0)
		{
			moduloCounter -= 1.0;
			return true;
		}

		// --- for negative frequencies
		if (phaseInc < 0 && moduloCounter <= 0.0)
		{
			moduloCounter += 1.0;
			return true;
		}

		return false;
	}

	/** advanvce the modulo counter */
	inline void advanceModulo(double& moduloCounter, double phaseInc) { moduloCounter += phaseInc; }

	const double B = 4.0 / kPi;
	const double C = -4.0 / (kPi* kPi);
	const double P = 0.225;
	/** parabolic sinusoidal calcualtion; NOTE: input is -pi to +pi http://devmaster.net/posts/9648/fast-and-accurate-sine-cosine */
	inline double parabolicSine(double angle)
	{
		double y = B * angle + C * angle * fabs(angle);
		y = P * (y * fabs(y) - y) + y;
		return y;
	}
};

/**
\enum DFOscillatorCoeffs
\ingroup Constants-Enums
\brief
Use this non-typed enum to easily access the direct form oscillator coefficients

- enum DFOscillatorCoeffs { df_b1, df_b2, numDFOCoeffs };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum DFOscillatorCoeffs { df_b1, df_b2, numDFOCoeffs };

/**
\enum DFOscillatorStates
\ingroup Constants-Enums
\brief
Use this non-typed enum to easily access the direct form oscillator state registers

- DFOscillatorStates { df_yz1, df_yz2, numDFOStates };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum DFOscillatorStates { df_yz1, df_yz2, numDFOStates };


/**
\class DFOscillator
\ingroup FX-Objects
\brief
The DFOscillator object implements generates a very pure sinusoidal oscillator by placing poles direclty on the unit circle.
Accuracy is excellent even at low frequencies.

Audio I/O:
- Output only object: pitched audio sinusoidal generator.

Control I/F:
- Use OscillatorParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class DFOscillator : public IAudioSignalGenerator
{
public:
	DFOscillator() { }	/* C-TOR */
	virtual ~DFOscillator() {}				/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		memset(&stateArray[0], 0, sizeof(double)*numDFOStates);
		updateDFO();
		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return OscillatorParameters custom data structure
	*/
	OscillatorParameters getParameters()
	{
		return parameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param OscillatorParameters custom data structure
	*/
	void setParameters(const OscillatorParameters& params)
	{
		if (parameters.frequency_Hz != params.frequency_Hz)
		{
			parameters = params;
			updateDFO();
		}
	}

	/** render the audio signal (pure sinusoid) */
	virtual const SignalGenData renderAudioOutput()
	{
		// --- calculates normal and inverted outputs; quadphase are not used
		SignalGenData output;

		// -- do difference equation y(n) = -b1y(n-2) - b2y(n-2)
		output.normalOutput = (-coeffArray[df_b1]*stateArray[df_yz1] - coeffArray[df_b2]*stateArray[df_yz2]);
		output.invertedOutput = -output.normalOutput;

		// --- update states
		stateArray[df_yz2] = stateArray[df_yz1];
		stateArray[df_yz1] = output.normalOutput;

		return output;
	}

	/** Update the object */
	void updateDFO()
	{
		// --- Oscillation Rate = theta = wT = w/fs
		double wT = (kTwoPi*parameters.frequency_Hz) / sampleRate;

		// --- coefficients to place poles right on unit circle
		coeffArray[df_b1] = -2.0*cos(wT);	// <--- set angle a = -2Rcod(theta)
		coeffArray[df_b2] = 1.0;			// <--- R^2 = 1, so R = 1

		// --- now update states to reflect the new frequency
		//	   re calculate the new initial conditions
		//	   arcsine of y(n-1) gives us wnT
		double wnT1 = asin(stateArray[df_yz1]);

		// find n by dividing wnT by wT
		double n = wnT1 / wT;

		// --- re calculate the new initial conditions
		//	   asin returns values from -pi/2 to +pi/2 where the sinusoid
		//     moves from -1 to +1 -- the leading (rising) edge of the
		//     sinewave. If we are on that leading edge (increasing)
		//     then we use the value 1T behind.
		//
		//     If we are on the falling edge, we use the value 1T ahead
		//     because it mimics the value that would be 1T behind
		if (stateArray[df_yz1] > stateArray[df_yz2])
			n -= 1;
		else
			n += 1;

		// ---  calculate the new (old) sample
		stateArray[df_yz2] = sin((n)*wT);
	}


protected:
	// --- parameters
	OscillatorParameters parameters; ///< object parameters

	// --- implementation of half a biquad - this object is extremely specific
	double stateArray[numDFOStates] = { 0.0 };///< array of state registers
	double coeffArray[numDFOCoeffs] = { 0.0 };///< array of coefficients

	// --- sample rate
	double sampleRate = 0.0;			///< sample rate
};


/**
\enum modDelaylgorithm
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set modulated delay algorithm.

- enum class modDelaylgorithm { kFlanger, kChorus, kVibrato };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class modDelaylgorithm { kFlanger, kChorus, kVibrato };


/**
\struct ModulatedDelayParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the ModulatedDelay object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct ModulatedDelayParameters
{
	ModulatedDelayParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	ModulatedDelayParameters& operator=(const ModulatedDelayParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		algorithm = params.algorithm;
		lfoRate_Hz = params.lfoRate_Hz;
		lfoDepth_Pct = params.lfoDepth_Pct;
		feedback_Pct = params.feedback_Pct;
		return *this;
	}

	// --- individual parameters
	modDelaylgorithm algorithm = modDelaylgorithm::kFlanger; ///< mod delay algorithm
	double lfoRate_Hz = 0.0;	///< mod delay LFO rate in Hz
	double lfoDepth_Pct = 0.0;	///< mod delay LFO depth in %
	double feedback_Pct = 0.0;	///< feedback in %
};

/**
\class ModulatedDelay
\ingroup FX-Objects
\brief
The ModulatedDelay object implements the three basic algorithms: flanger, chorus, vibrato.

Audio I / O :
	-Processes mono input to mono OR stereo output.

Control I / F :
	-Use ModulatedDelayParameters structure to get / set object params.

\author Will Pirkle http ://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed.by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class ModulatedDelay : public IAudioSignalProcessor
{
public:
	ModulatedDelay() {
	}		/* C-TOR */
	~ModulatedDelay() {}	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- create new buffer, 100mSec long
		delay.reset(_sampleRate);
		delay.createDelayBuffers(_sampleRate, 100.0);

		// --- lfo
		lfo.reset(_sampleRate);
		OscillatorParameters params = lfo.getParameters();
		params.waveform = generatorWaveform::kTriangle;
		lfo.setParameters(params);

		return true;
	}

	/** process input sample */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		float input = xn;
		float output = 0.0;
		processAudioFrame(&input, &output, 1, 1);
		return output;
	}

	/** return true: this object can process frames */
	virtual bool canProcessAudioFrame() { return true; }

	/** process STEREO audio delay of frames */
	virtual bool processAudioFrame(const float* inputFrame,		/* ptr to one frame of data: pInputFrame[0] = left, pInputFrame[1] = right, etc...*/
		float* outputFrame,
		uint32_t inputChannels,
		uint32_t outputChannels)
	{
		// --- make sure we have input and outputs
		if (inputChannels == 0 || outputChannels == 0)
			return false;

		// --- render LFO
		SignalGenData lfoOutput = lfo.renderAudioOutput();

		// --- setup delay modulation
		AudioDelayParameters params = delay.getParameters();
		double minDelay_mSec = 0.0;
		double maxDepth_mSec = 0.0;

		// --- set delay times, wet/dry and feedback
		if (parameters.algorithm == modDelaylgorithm::kFlanger)
		{
			minDelay_mSec = 0.1;
			maxDepth_mSec = 7.0;
			params.wetLevel_dB = -3.0;
			params.dryLevel_dB = -3.0;
		}
		if (parameters.algorithm == modDelaylgorithm::kChorus)
		{
			minDelay_mSec = 10.0;
			maxDepth_mSec = 30.0;
			params.wetLevel_dB = -3.0;
			params.dryLevel_dB = -0.0;
			params.feedback_Pct = 0.0;
		}
		if (parameters.algorithm == modDelaylgorithm::kVibrato)
		{
			minDelay_mSec = 0.0;
			maxDepth_mSec = 7.0;
			params.wetLevel_dB = 0.0;
			params.dryLevel_dB = -96.0;
			params.feedback_Pct = 0.0;
		}

		// --- calc modulated delay times
		double depth = parameters.lfoDepth_Pct / 100.0;
		double modulationMin = minDelay_mSec;
		double modulationMax = minDelay_mSec + maxDepth_mSec;

		// --- flanger - unipolar
		if (parameters.algorithm == modDelaylgorithm::kFlanger)
			params.leftDelay_mSec = doUnipolarModulationFromMin(bipolarToUnipolar(depth * lfoOutput.normalOutput),
															     modulationMin, modulationMax);
		else
			params.leftDelay_mSec = doBipolarModulation(depth * lfoOutput.normalOutput, modulationMin, modulationMax);


		// --- set right delay to match (*Hint Homework!)
		params.rightDelay_mSec = params.leftDelay_mSec;

		// --- modulate the delay
		delay.setParameters(params);

		// --- just call the function and pass our info in/out
		return delay.processAudioFrame(inputFrame, outputFrame, inputChannels, outputChannels);
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return ModulatedDelayParameters custom data structure
	*/
	ModulatedDelayParameters getParameters() { return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param ModulatedDelayParameters custom data structure
	*/
	void setParameters(ModulatedDelayParameters _parameters)
	{
		// --- bulk copy
		parameters = _parameters;

		OscillatorParameters lfoParams = lfo.getParameters();
		lfoParams.frequency_Hz = parameters.lfoRate_Hz;
		if (parameters.algorithm == modDelaylgorithm::kVibrato)
			lfoParams.waveform = generatorWaveform::kSin;
		else
			lfoParams.waveform = generatorWaveform::kTriangle;

		lfo.setParameters(lfoParams);

		AudioDelayParameters adParams = delay.getParameters();
		adParams.feedback_Pct = parameters.feedback_Pct;
		delay.setParameters(adParams);
	}

private:
	ModulatedDelayParameters parameters; ///< object parameters
	AudioDelay delay;	///< the delay to modulate
	LFO lfo;			///< the modulator
};

/**
\struct PhaseShifterParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the PhaseShifter object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct PhaseShifterParameters
{
	PhaseShifterParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	PhaseShifterParameters& operator=(const PhaseShifterParameters& params)
	{
		if (this == &params)
			return *this;

		lfoRate_Hz = params.lfoRate_Hz;
		lfoDepth_Pct = params.lfoDepth_Pct;
		intensity_Pct = params.intensity_Pct;
		quadPhaseLFO = params.quadPhaseLFO;
		return *this;
	}

	// --- individual parameters
	double lfoRate_Hz = 0.0;	///< phaser LFO rate in Hz
	double lfoDepth_Pct = 0.0;	///< phaser LFO depth in %
	double intensity_Pct = 0.0;	///< phaser feedback in %
	bool quadPhaseLFO = false;	///< quad phase LFO flag
};

// --- constants for Phaser
const unsigned int PHASER_STAGES = 6;

// --- these are the ideal band definitions
//const double apf0_minF = 16.0;
//const double apf0_maxF = 1600.0;
//
//const double apf1_minF = 33.0;
//const double apf1_maxF = 3300.0;
//
//const double apf2_minF = 48.0;
//const double apf2_maxF = 4800.0;
//
//const double apf3_minF = 98.0;
//const double apf3_maxF = 9800.0;
//
//const double apf4_minF = 160.0;
//const double apf4_maxF = 16000.0;
//
//const double apf5_minF = 260.0;
//const double apf5_maxF = 20480.0;

// --- these are the exact values from the National Semiconductor Phaser design
const double apf0_minF = 32.0;
const double apf0_maxF = 1500.0;

const double apf1_minF = 68.0;
const double apf1_maxF = 3400.0;

const double apf2_minF = 96.0;
const double apf2_maxF = 4800.0;

const double apf3_minF = 212.0;
const double apf3_maxF = 10000.0;

const double apf4_minF = 320.0;
const double apf4_maxF = 16000.0;

const double apf5_minF = 636.0;
const double apf5_maxF = 20480.0;

/**
\class PhaseShifter
\ingroup FX-Objects
\brief
The PhaseShifter object implements a six-stage phaser.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use BiquadParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class PhaseShifter : public IAudioSignalProcessor
{
public:
	PhaseShifter(void) {
		OscillatorParameters lfoparams = lfo.getParameters();
		lfoparams.waveform = generatorWaveform::kTriangle;	// kTriangle LFO for phaser
	//	lfoparams.waveform = generatorWaveform::kSin;		// kTriangle LFO for phaser
		lfo.setParameters(lfoparams);

		AudioFilterParameters params = apf[0].getParameters();
		params.algorithm = filterAlgorithm::kAPF1; // can also use 2nd order
		// params.Q = 0.001; use low Q if using 2nd order APFs

		for (int i = 0; i < PHASER_STAGES; i++)
		{
			apf[i].setParameters(params);
		}
	}	/* C-TOR */

	~PhaseShifter(void) {}	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- reset LFO
		lfo.reset(_sampleRate);

		// --- reset APFs
		for (int i = 0; i < PHASER_STAGES; i++){
			apf[i].reset(_sampleRate);
		}

		return true;
	}

	/** process autio through phaser */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		SignalGenData lfoData = lfo.renderAudioOutput();

		// --- create the bipolar modulator value
		double lfoValue = lfoData.normalOutput;
		if (parameters.quadPhaseLFO)
			lfoValue = lfoData.quadPhaseOutput_pos;

		double depth = parameters.lfoDepth_Pct / 100.0;
		double modulatorValue = lfoValue*depth;

		// --- calculate modulated values for each APF; note they have different ranges
		AudioFilterParameters params = apf[0].getParameters();
		params.fc = doBipolarModulation(modulatorValue, apf0_minF, apf0_maxF);
		apf[0].setParameters(params);

		params = apf[1].getParameters();
		params.fc = doBipolarModulation(modulatorValue, apf1_minF, apf1_maxF);
		apf[1].setParameters(params);

		params = apf[2].getParameters();
		params.fc = doBipolarModulation(modulatorValue, apf2_minF, apf2_maxF);
		apf[2].setParameters(params);

		params = apf[3].getParameters();
		params.fc = doBipolarModulation(modulatorValue, apf3_minF, apf3_maxF);
		apf[3].setParameters(params);

		params = apf[4].getParameters();
		params.fc = doBipolarModulation(modulatorValue, apf4_minF, apf4_maxF);
		apf[4].setParameters(params);

		params = apf[5].getParameters();
		params.fc = doBipolarModulation(modulatorValue, apf5_minF, apf5_maxF);
		apf[5].setParameters(params);

		// --- calculate gamma values
		double gamma1 = apf[5].getG_value();
		double gamma2 = apf[4].getG_value() * gamma1;
		double gamma3 = apf[3].getG_value() * gamma2;
		double gamma4 = apf[2].getG_value() * gamma3;
		double gamma5 = apf[1].getG_value() * gamma4;
		double gamma6 = apf[0].getG_value() * gamma5;

		// --- set the alpha0 value
		double K = parameters.intensity_Pct / 100.0;
		double alpha0 = 1.0 / (1.0 + K*gamma6);

		// --- create combined feedback
		double Sn = gamma5*apf[0].getS_value() + gamma4*apf[1].getS_value() + gamma3*apf[2].getS_value() + gamma2*apf[3].getS_value() + gamma1*apf[4].getS_value() + apf[5].getS_value();

		// --- form input to first APF
		double u = alpha0*(xn + K*Sn);

		// --- cascade of APFs (could also nest these in one massive line of code)
		double APF1 = apf[0].processAudioSample(u);
		double APF2 = apf[1].processAudioSample(APF1);
		double APF3 = apf[2].processAudioSample(APF2);
		double APF4 = apf[3].processAudioSample(APF3);
		double APF5 = apf[4].processAudioSample(APF4);
		double APF6 = apf[5].processAudioSample(APF5);

		// --- sum with -3dB coefficients
		//	double output = 0.707*xn + 0.707*APF6;

		// --- sum with National Semiconductor design ratio:
		//	   dry = 0.5, wet = 5.0
		// double output = 0.5*xn + 5.0*APF6;
		// double output = 0.25*xn + 2.5*APF6;
		double output = 0.125*xn + 1.25*APF6;

		return output;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return PhaseShifterParameters custom data structure
	*/
	PhaseShifterParameters getParameters() { return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param PhaseShifterParameters custom data structure
	*/
	void setParameters(const PhaseShifterParameters& params)
	{
		// --- update LFO rate
		if (params.lfoRate_Hz != parameters.lfoRate_Hz)
		{
			OscillatorParameters lfoparams = lfo.getParameters();
			lfoparams.frequency_Hz = params.lfoRate_Hz;
			lfo.setParameters(lfoparams);
		}

		// --- save new
		parameters = params;
	}
protected:
	PhaseShifterParameters parameters;  ///< the object parameters
	AudioFilter apf[PHASER_STAGES];		///< six APF objects
	LFO lfo;							///< the one and only LFO
};

/**
\struct SimpleLPFParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the SimpleLPFP object. Used for reverb algorithms in book.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct SimpleLPFParameters
{
	SimpleLPFParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	SimpleLPFParameters& operator=(const SimpleLPFParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		g = params.g;
		return *this;
	}

	// --- individual parameters
	double g = 0.0; ///< simple LPF g value
};

/**
\class SimpleLPF
\ingroup FX-Objects
\brief
The SimpleLPF object implements a first order one-pole LPF using one coefficient "g" value.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use SimpleLPFParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class SimpleLPF : public IAudioSignalProcessor
{
public:
	SimpleLPF(void) {}	/* C-TOR */
	~SimpleLPF(void) {}	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		state = 0.0;
		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return SimpleLPFParameters custom data structure
	*/
	SimpleLPFParameters getParameters()
	{
		return simpleLPFParameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param SimpleLPFParameters custom data structure
	*/
	void setParameters(const SimpleLPFParameters& params)
	{
		simpleLPFParameters = params;
	}

	/** process simple one pole FB back filter */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		double g = simpleLPFParameters.g;
		double yn = (1.0 - g)*xn + g*state;
		state = yn;
		return yn;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

private:
	SimpleLPFParameters simpleLPFParameters;	///< object parameters
	double state = 0.0;							///< single state (z^-1) register
};

/**
\struct SimpleDelayParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the SimpleDelay object. Used for reverb algorithms in book.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct SimpleDelayParameters
{
	SimpleDelayParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	SimpleDelayParameters& operator=(const SimpleDelayParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		delayTime_mSec = params.delayTime_mSec;
		interpolate = params.interpolate;
		delay_Samples = params.delay_Samples;
		return *this;
	}

	// --- individual parameters
	double delayTime_mSec = 0.0;	///< delay tine in mSec
	bool interpolate = false;		///< interpolation flag (diagnostics usually)

	// --- outbound parameters
	double delay_Samples = 0.0;		///< current delay in samples; other objects may need to access this information
};

/**
\class SimpleDelay
\ingroup FX-Objects
\brief
The SimpleDelay object implements a basic delay line without feedback.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use SimpleDelayParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class SimpleDelay : public IAudioSignalProcessor
{
public:
	SimpleDelay(void) {}	/* C-TOR */
	~SimpleDelay(void) {}	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- if sample rate did not change
		if (sampleRate == _sampleRate)
		{
			// --- just flush buffer and return
			delayBuffer.flushBuffer();
			return true;
		}

		// --- create new buffer, will store sample rate and length(mSec)
		createDelayBuffer(_sampleRate, bufferLength_mSec);

		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return SimpleDelayParameters custom data structure
	*/
	SimpleDelayParameters getParameters()
	{
		return simpleDelayParameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param SimpleDelayParameters custom data structure
	*/
	void setParameters(const SimpleDelayParameters& params)
	{
		simpleDelayParameters = params;
		simpleDelayParameters.delay_Samples = simpleDelayParameters.delayTime_mSec*(samplesPerMSec);
		delayBuffer.setInterpolate(simpleDelayParameters.interpolate);
	}

	/** process MONO audio delay */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- read delay
		if (simpleDelayParameters.delay_Samples == 0)
			return xn;

		double yn = delayBuffer.readBuffer(simpleDelayParameters.delay_Samples);

		// --- write to delay buffer
		delayBuffer.writeBuffer(xn);

		// --- done
		return yn;
	}

	/** reset members to initialized state */
	virtual bool canProcessAudioFrame() { return false; }

	/** create a new delay buffer */
	void createDelayBuffer(double _sampleRate, double _bufferLength_mSec)
	{
		// --- store for math
		bufferLength_mSec = _bufferLength_mSec;
		sampleRate = _sampleRate;
		samplesPerMSec = sampleRate / 1000.0;

		// --- total buffer length including fractional part
		bufferLength = (unsigned int)(bufferLength_mSec*(samplesPerMSec)) + 1; // +1 for fractional part

		// --- create new buffer
		delayBuffer.createCircularBuffer(bufferLength);
	}

	/** read delay at current location */
	double readDelay()
	{
		// --- simple read
		return delayBuffer.readBuffer(simpleDelayParameters.delay_Samples);
	}

	/** read delay at current location */
	double readDelayAtTime_mSec(double _delay_mSec)
	{
		// --- calculate total delay time in samples + fraction
		double _delay_Samples = _delay_mSec*(samplesPerMSec);

		// --- simple read
		return delayBuffer.readBuffer(_delay_Samples);
	}

	/** read delay at a percentage of total length */
	double readDelayAtPercentage(double delayPercent)
	{
		// --- simple read
		return delayBuffer.readBuffer((delayPercent / 100.0)*simpleDelayParameters.delay_Samples);
	}

	/** write a new value into the delay */
	void writeDelay(double xn)
	{
		// --- simple write
		delayBuffer.writeBuffer(xn);
	}

private:
	SimpleDelayParameters simpleDelayParameters; ///< object parameters

	double sampleRate = 0.0;		///< sample rate
	double samplesPerMSec = 0.0;	///< samples per millisecond (for arbitrary access)
	double bufferLength_mSec = 0.0; ///< total buffer lenth in mSec
	unsigned int bufferLength = 0;	///< buffer length in samples

	// --- delay buffer of doubles
	CircularBuffer<double> delayBuffer; ///< circular buffer for delay
};


/**
\struct CombFilterParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the CombFilter object. Used for reverb algorithms in book.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct CombFilterParameters
{
	CombFilterParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	CombFilterParameters& operator=(const CombFilterParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		delayTime_mSec = params.delayTime_mSec;
		RT60Time_mSec = params.RT60Time_mSec;
		enableLPF = params.enableLPF;
		lpf_g = params.lpf_g;
		interpolate = params.interpolate;
		return *this;
	}

	// --- individual parameters
	double delayTime_mSec = 0.0;	///< delay time in mSec
	double RT60Time_mSec = 0.0;		///< RT 60 time ini mSec
	bool enableLPF = false;			///< enable LPF flag
	double lpf_g = 0.0;				///< gain value for LPF (if enabled)
	bool interpolate = false;		///< interpolation flag (diagnostics)
};


/**
\class CombFilter
\ingroup FX-Objects
\brief
The CombFilter object implements a comb filter with optional LPF in feedback loop. Used for reverb algorithms in book.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use CombFilterParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class CombFilter : public IAudioSignalProcessor
{
public:
	CombFilter(void) {}		/* C-TOR */
	~CombFilter(void) {}	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- flush
		lpf_state = 0.0;

		// --- create new buffer, will store sample rate and length(mSec)
		createDelayBuffer(sampleRate, bufferLength_mSec);

		return true;
	}

	/** process CombFilter */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		double yn = delay.readDelay();
		double input = 0.0;

		// --- form input & write
		if (combFilterParameters.enableLPF)
		{
			// --- apply simple 1st order pole LPF
			double g2 = lpf_g*(1.0 - comb_g); // see book for equation 11.27 (old book)
			double filteredSignal = yn + g2*lpf_state;
			input = xn + comb_g*(filteredSignal);
			lpf_state = filteredSignal;
		}
		else
		{
			input = xn + comb_g*yn;
		}

		delay.writeDelay(input);

		// --- done
		return yn;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return CombFilterParameters custom data structure
	*/
	CombFilterParameters getParameters()
	{
		return combFilterParameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param CombFilterParameters custom data structure
	*/
	void setParameters(const CombFilterParameters& params)
	{
		combFilterParameters = params;

		// --- update the delay line parameters first
		SimpleDelayParameters delayParams = delay.getParameters();
		delayParams.delayTime_mSec = combFilterParameters.delayTime_mSec;
		delayParams.interpolate = combFilterParameters.interpolate;
		delay.setParameters(delayParams); // this will set the delay time in samples

										  // --- calculate g with RT60 time (requires updated delay above^^)
		double exponent = -3.0*delayParams.delay_Samples*(1.0 / sampleRate);
		double rt60_mSec = combFilterParameters.RT60Time_mSec / 1000.0; // RT is in mSec!
		comb_g = pow(10.0, exponent / rt60_mSec);

		// --- set LPF g
		lpf_g = combFilterParameters.lpf_g;
	}

	/** create new buffers */
	void createDelayBuffer(double _sampleRate, double delay_mSec)
	{
		sampleRate = _sampleRate;
		bufferLength_mSec = delay_mSec;

		// --- create new buffer, will store sample rate and length(mSec)
		delay.createDelayBuffer(_sampleRate, delay_mSec);
	}

private:
	CombFilterParameters combFilterParameters; ///< object parameters
	double sampleRate = 0.0;	///< sample rate
	double comb_g = 0.0;		///< g value for comb filter (feedback, not %)
	double bufferLength_mSec = 0.0; ///< total buffer length

	// --- LPF support
	double lpf_g = 0.0;		///< LPF g value
	double lpf_state = 0.0; ///< state register for LPF (z^-1)

	// --- delay buffer of doubles
	SimpleDelay delay;		///< delay for comb filter
};

/**
\struct DelayAPFParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the DelayAPF object. Used for reverb algorithms in book.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct DelayAPFParameters
{
	DelayAPFParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	DelayAPFParameters& operator=(const DelayAPFParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		delayTime_mSec = params.delayTime_mSec;
		apf_g = params.apf_g;
		enableLPF = params.enableLPF;
		lpf_g = params.lpf_g;
		interpolate = params.interpolate;
		enableLFO = params.enableLFO;
		lfoRate_Hz = params.lfoRate_Hz;
		lfoDepth = params.lfoDepth;
		lfoMaxModulation_mSec = params.lfoMaxModulation_mSec;
		return *this;
	}

	// --- individual parameters
	double delayTime_mSec = 0.0;	///< APF delay time
	double apf_g = 0.0;				///< APF g coefficient
	bool enableLPF = false;			///< flag to enable LPF in structure
	double lpf_g = 0.0;				///< LPF g coefficient (if enabled)
	bool interpolate = false;		///< interpolate flag (diagnostics)
	bool enableLFO = false;			///< flag to enable LFO
	double lfoRate_Hz = 0.0;		///< LFO rate in Hz, if enabled
	double lfoDepth = 0.0;			///< LFO deoth (not in %) if enabled
	double lfoMaxModulation_mSec = 0.0;	///< LFO maximum modulation time in mSec

};

/**
\class DelayAPF
\ingroup FX-Objects
\brief
The DelayAPF object implements a delaying APF with optional LPF and optional modulated delay time with LFO.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use DelayAPFParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class DelayAPF : public IAudioSignalProcessor
{
public:
	DelayAPF(void) {}	/* C-TOR */
	~DelayAPF(void) {}	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- reset children
		modLFO.reset(_sampleRate);

		// --- flush
		lpf_state = 0.0;

		// --- create new buffer, will store sample rate and length(mSec)
		createDelayBuffer(sampleRate, bufferLength_mSec);

		return true;
	}

	/** process one input sample through object */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		SimpleDelayParameters delayParams = delay.getParameters();
		if (delayParams.delay_Samples == 0)
			return xn;

		// --- delay line output
		double wnD = 0.0;
		double apf_g = delayAPFParameters.apf_g;
		double lpf_g = delayAPFParameters.lpf_g;
		double lfoDepth = delayAPFParameters.lfoDepth;

		// --- for modulated APFs
		if (delayAPFParameters.enableLFO)
		{
			SignalGenData lfoOutput = modLFO.renderAudioOutput();
			double maxDelay = delayParams.delayTime_mSec;
			double minDelay = maxDelay - delayAPFParameters.lfoMaxModulation_mSec;
			minDelay = fmax(0.0, minDelay); // bound minDelay to 0 as minimum

			// --- calc max-down modulated value with unipolar converted LFO output
			//     NOTE: LFO output is scaled by lfoDepth
			double modDelay_mSec = doUnipolarModulationFromMax(bipolarToUnipolar(lfoDepth*lfoOutput.normalOutput),
				minDelay, maxDelay);

			// --- read modulated value to get w(n-D);
			wnD = delay.readDelayAtTime_mSec(modDelay_mSec);
		}
		else
			// --- read the delay line to get w(n-D)
			wnD = delay.readDelay();

		if (delayAPFParameters.enableLPF)
		{
			// --- apply simple 1st order pole LPF, overwrite wnD
			wnD = wnD*(1.0 - lpf_g) + lpf_g*lpf_state;
			lpf_state = wnD;
		}

		// form w(n) = x(n) + gw(n-D)
		double wn = xn + apf_g*wnD;

		// form y(n) = -gw(n) + w(n-D)
		double yn = -apf_g*wn + wnD;

		// underflow check
		checkFloatUnderflow(yn);

		// write delay line
		delay.writeDelay(wn);

		return yn;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return DelayAPFParameters custom data structure
	*/
	DelayAPFParameters getParameters()
	{
		return delayAPFParameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param DelayAPFParameters custom data structure
	*/
	void setParameters(const DelayAPFParameters& params)
	{
		delayAPFParameters = params;

		// --- update delay line
		SimpleDelayParameters delayParams = delay.getParameters();
		delayParams.delayTime_mSec = delayAPFParameters.delayTime_mSec;
		delay.setParameters(delayParams);
	}

	/** create the delay buffer in mSec */
	void createDelayBuffer(double _sampleRate, double delay_mSec)
	{
		sampleRate = _sampleRate;
		bufferLength_mSec = delay_mSec;

		// --- create new buffer, will store sample rate and length(mSec)
		delay.createDelayBuffer(_sampleRate, delay_mSec);
	}

protected:
	// --- component parameters
	DelayAPFParameters delayAPFParameters;	///< obeject parameters
	double sampleRate = 0.0;				///< current sample rate
	double bufferLength_mSec = 0.0;			///< total buffer length in mSec

	// --- delay buffer of doubles
	SimpleDelay delay;						///< delay

	// --- optional LFO
	LFO modLFO;								///< LFO

	// --- LPF support
	double lpf_state = 0.0;					///< LPF state register (z^-1)
};


/**
\struct NestedDelayAPFParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the NestedDelayAPF object. Used for reverb algorithms in book.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct NestedDelayAPFParameters
{
	NestedDelayAPFParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	NestedDelayAPFParameters& operator=(const NestedDelayAPFParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		outerAPFdelayTime_mSec = params.outerAPFdelayTime_mSec;
		innerAPFdelayTime_mSec = params.innerAPFdelayTime_mSec;
		outerAPF_g = params.outerAPF_g;
		innerAPF_g = params.innerAPF_g;

		// --- outer LFO
		enableLFO = params.enableLFO;
		lfoRate_Hz = params.lfoRate_Hz;
		lfoDepth = params.lfoDepth;
		lfoMaxModulation_mSec = params.lfoMaxModulation_mSec;

		return *this;
	}

	// --- individual parameters
	double outerAPFdelayTime_mSec = 0.0;	///< delay time for outer APF
	double innerAPFdelayTime_mSec = 0.0;	///< delay time for inner APF
	double outerAPF_g = 0.0;				///< g coefficient for outer APF
	double innerAPF_g = 0.0;				///< g coefficient for inner APF

	// --- this LFO belongs to the outer APF only
	bool enableLFO = false;					///< flag to enable the modulated delay
	double lfoRate_Hz = 0.0;				///< LFO rate in Hz (if enabled)
	double lfoDepth = 1.0;					///< LFO depth (not in %) (if enabled)
	double lfoMaxModulation_mSec = 0.0;		///< max modulation time if LFO is enabled

};

/**
\class NestedDelayAPF
\ingroup FX-Objects
\brief
The NestedDelayAPF object implements a pair of nested Delaying APF structures. These are labled the
outer and inner APFs. The outer APF's LPF and LFO may be optionally enabled. You might want
to extend this object to enable and use the inner LPF and LFO as well.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use BiquadParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class NestedDelayAPF : public DelayAPF
{
public:
	NestedDelayAPF(void) { }	/* C-TOR */
	~NestedDelayAPF(void) { }	/* D-TOR */

public:
	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- call base class reset first
		DelayAPF::reset(_sampleRate);

		// --- then do our stuff
		nestedAPF.reset(_sampleRate);

		return true;
	}

	/** process mono audio input */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- delay line output
		double wnD = 0.0;

		SimpleDelayParameters delayParams = delay.getParameters();
		if (delayParams.delay_Samples == 0)
			return xn;

		double apf_g = delayAPFParameters.apf_g;
		double lpf_g = delayAPFParameters.lpf_g;

		// --- for modulated APFs
		if (delayAPFParameters.enableLFO)
		{
			SignalGenData lfoOutput = modLFO.renderAudioOutput();
			double maxDelay = delayParams.delayTime_mSec;
			double minDelay = maxDelay - delayAPFParameters.lfoMaxModulation_mSec;
			minDelay = fmax(0.0, minDelay); // bound minDelay to 0 as minimum
			double lfoDepth = delayAPFParameters.lfoDepth;

			// --- calc max-down modulated value with unipolar converted LFO output
			//     NOTE: LFO output is scaled by lfoDepth
			double modDelay_mSec = doUnipolarModulationFromMax(bipolarToUnipolar(lfoDepth*lfoOutput.normalOutput),
				minDelay, maxDelay);

			// --- read modulated value to get w(n-D);
			wnD = delay.readDelayAtTime_mSec(modDelay_mSec);
		}
		else
			// --- read the delay line to get w(n-D)
			wnD = delay.readDelay();

		if (delayAPFParameters.enableLPF)
		{
			// --- apply simple 1st order pole LPF, overwrite wnD
			wnD = wnD*(1.0 - lpf_g) + lpf_g*lpf_state;
			lpf_state = wnD;
		}

		// --- form w(n) = x(n) + gw(n-D)
		double wn = xn + apf_g*wnD;

		// --- process wn through inner APF
		double ynInner = nestedAPF.processAudioSample(wn);

		// --- form y(n) = -gw(n) + w(n-D)
		double yn = -apf_g*wn + wnD;

		// --- underflow check
		checkFloatUnderflow(yn);

		// --- write delay line
		delay.writeDelay(ynInner);

		return yn;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return BiquadParameters custom data structure
	*/
	NestedDelayAPFParameters getParameters() { return nestedAPFParameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param BiquadParameters custom data structure
	*/
	void setParameters(const NestedDelayAPFParameters& params)
	{
		nestedAPFParameters = params;

		DelayAPFParameters outerAPFParameters = DelayAPF::getParameters();
		DelayAPFParameters innerAPFParameters = nestedAPF.getParameters();

		// --- outer APF
		outerAPFParameters.apf_g = nestedAPFParameters.outerAPF_g;
		outerAPFParameters.delayTime_mSec = nestedAPFParameters.outerAPFdelayTime_mSec;

		// --- LFO support
		outerAPFParameters.enableLFO = nestedAPFParameters.enableLFO;
		outerAPFParameters.lfoDepth = nestedAPFParameters.lfoDepth;
		outerAPFParameters.lfoRate_Hz = nestedAPFParameters.lfoRate_Hz;
		outerAPFParameters.lfoMaxModulation_mSec = nestedAPFParameters.lfoMaxModulation_mSec;

		// --- inner APF
		innerAPFParameters.apf_g = nestedAPFParameters.innerAPF_g;
		innerAPFParameters.delayTime_mSec = nestedAPFParameters.innerAPFdelayTime_mSec;

		DelayAPF::setParameters(outerAPFParameters);
		nestedAPF.setParameters(innerAPFParameters);
	}

	/** createDelayBuffers -- note there are two delay times here for inner and outer APFs*/
	void createDelayBuffers(double _sampleRate, double delay_mSec, double nestedAPFDelay_mSec)
	{
		// --- base class
		DelayAPF::createDelayBuffer(_sampleRate, delay_mSec);

		// --- then our stuff
		nestedAPF.createDelayBuffer(_sampleRate, nestedAPFDelay_mSec);
	}

private:
	NestedDelayAPFParameters nestedAPFParameters; ///< object parameters
	DelayAPF nestedAPF;	///< nested APF object
};

/**
\struct TwoBandShelvingFilterParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the TwoBandShelvingFilter object. Used for reverb algorithms in book.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct TwoBandShelvingFilterParameters
{
	TwoBandShelvingFilterParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	TwoBandShelvingFilterParameters& operator=(const TwoBandShelvingFilterParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		lowShelf_fc = params.lowShelf_fc;
		lowShelfBoostCut_dB = params.lowShelfBoostCut_dB;
		highShelf_fc = params.highShelf_fc;
		highShelfBoostCut_dB = params.highShelfBoostCut_dB;
		return *this;
	}

	// --- individual parameters
	double lowShelf_fc = 0.0;			///< fc for low shelf
	double lowShelfBoostCut_dB = 0.0;	///< low shelf gain
	double highShelf_fc = 0.0;			///< fc for high shelf
	double highShelfBoostCut_dB = 0.0;	///< high shelf gain
};

/**
\class TwoBandShelvingFilter
\ingroup FX-Objects
\brief
The TwoBandShelvingFilter object implements two shelving filters in series in the standard "Bass and Treble" configuration.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use TwoBandShelvingFilterParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class TwoBandShelvingFilter : public IAudioSignalProcessor
{
public:
	TwoBandShelvingFilter()
	{
		AudioFilterParameters params = lowShelfFilter.getParameters();
		params.algorithm = filterAlgorithm::kLowShelf;
		lowShelfFilter.setParameters(params);

		params = highShelfFilter.getParameters();
		params.algorithm = filterAlgorithm::kHiShelf;
		highShelfFilter.setParameters(params);
	}		/* C-TOR */

	~TwoBandShelvingFilter() {}		/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		lowShelfFilter.reset(_sampleRate);
		highShelfFilter.reset(_sampleRate);
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process a single input through the two filters in series */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- all modes do Full Wave Rectification
		double filteredSignal = lowShelfFilter.processAudioSample(xn);
		filteredSignal = highShelfFilter.processAudioSample(filteredSignal);

		return filteredSignal;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return TwoBandShelvingFilterParameters custom data structure
	*/
	TwoBandShelvingFilterParameters getParameters()
	{
		return parameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param TwoBandShelvingFilterParameters custom data structure
	*/
	void setParameters(const TwoBandShelvingFilterParameters& params)
	{
		parameters = params;
		AudioFilterParameters filterParams = lowShelfFilter.getParameters();
		filterParams.fc = parameters.lowShelf_fc;
		filterParams.boostCut_dB = parameters.lowShelfBoostCut_dB;
		lowShelfFilter.setParameters(filterParams);

		filterParams = highShelfFilter.getParameters();
		filterParams.fc = parameters.highShelf_fc;
		filterParams.boostCut_dB = parameters.highShelfBoostCut_dB;
		highShelfFilter.setParameters(filterParams);
	}

private:
	TwoBandShelvingFilterParameters parameters; ///< object parameters
	AudioFilter lowShelfFilter;					///< filter for low shelf
	AudioFilter highShelfFilter;				///< filter for high shelf
};

/**
\enum reverbDensity
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the density in the reverb object.

- enum class reverbDensity { kThick, kSparse };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class reverbDensity { kThick, kSparse };

/**
\struct ReverbTankParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the ReverbTank object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct ReverbTankParameters
{
	ReverbTankParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	ReverbTankParameters& operator=(const ReverbTankParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		density = params.density;

		// --- tweaker variables
		apfDelayMax_mSec = params.apfDelayMax_mSec;
		apfDelayWeight_Pct = params.apfDelayWeight_Pct;
		fixeDelayMax_mSec = params.fixeDelayMax_mSec;
		fixeDelayWeight_Pct = params.fixeDelayWeight_Pct;
		preDelayTime_mSec = params.preDelayTime_mSec;

		lpf_g = params.lpf_g;
		kRT = params.kRT;

		lowShelf_fc = params.lowShelf_fc;
		lowShelfBoostCut_dB = params.lowShelfBoostCut_dB;
		highShelf_fc = params.highShelf_fc;
		highShelfBoostCut_dB = params.highShelfBoostCut_dB;

		wetLevel_dB = params.wetLevel_dB;
		dryLevel_dB = params.dryLevel_dB;
		return *this;
	}

	// --- individual parameters
	reverbDensity density = reverbDensity::kThick;	///< density setting thick or thin

	// --- tweaking parameters - you may not want to expose these
	//     in the final plugin!
	// --- See the book for all the details on how these tweakers work!!
	double apfDelayMax_mSec = 5.0;					///< APF max delay time
	double apfDelayWeight_Pct = 100.0;				///< APF max delay weighying
	double fixeDelayMax_mSec = 50.0;				///< fixed delay max time
	double fixeDelayWeight_Pct = 100.0;				///< fixed delay max weighying

	// --- direct control parameters
	double preDelayTime_mSec = 0.0;					///< pre-delay time in mSec
	double lpf_g = 0.0;								///< LPF g coefficient
	double kRT = 0.0;								///< reverb time, 0 to 1

	double lowShelf_fc = 0.0;						///< low shelf fc
	double lowShelfBoostCut_dB = 0.0;				///< low shelf gain
	double highShelf_fc = 0.0;						///< high shelf fc
	double highShelfBoostCut_dB = 0.0;				///< high shelf gain

	double wetLevel_dB = -3.0;						///< wet output level in dB
	double dryLevel_dB = -3.0;						///< dry output level in dB
};

// --- constants for reverb tank
const unsigned int NUM_BRANCHES = 4;
const unsigned int NUM_CHANNELS = 2; // stereo

/**
\class ReverbTank
\ingroup FX-Objects
\brief
The ReverbTank object implements the cyclic reverb tank in the FX book listed below.

Audio I/O:
- Processes mono input to mono OR stereo output.

Control I/F:
- Use ReverbTankParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class ReverbTank : public IAudioSignalProcessor
{
public:
	ReverbTank() {}		/* C-TOR */
	~ReverbTank() {}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// ---store
		sampleRate = _sampleRate;

		// ---set up preDelay
		preDelay.reset(_sampleRate);
		preDelay.createDelayBuffer(_sampleRate, 100.0);

		for (int i = 0; i < NUM_BRANCHES; i++)
		{
			branchDelays[i].reset(_sampleRate);
			branchDelays[i].createDelayBuffer(_sampleRate, 100.0);

			branchNestedAPFs[i].reset(_sampleRate);
			branchNestedAPFs[i].createDelayBuffers(_sampleRate, 100.0, 100.0);

			branchLPFs[i].reset(_sampleRate);
		}
		for (int i = 0; i < NUM_CHANNELS; i++)
		{
			shelvingFilters[i].reset(_sampleRate);
		}

		return true;
	}

	/** return true: this object can process frames */
	virtual bool canProcessAudioFrame() { return true; }

	/** process mono reverb tank */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		float inputs[2] = { 0.0 };
		float outputs[2] = { 0.0 };
		processAudioFrame(inputs, outputs, 1, 1);
		return outputs[0];
	}

	/** process stereo reverb tank */
	virtual bool processAudioFrame(const float* inputFrame,
		float* outputFrame,
		uint32_t inputChannels,
		uint32_t outputChannels)
	{
		double output = 0.0;

		// --- global feedback from delay in last branch
		double globFB = branchDelays[NUM_BRANCHES-1].readDelay();

		// --- feedback value
		double fb = parameters.kRT*(globFB);

		// --- mono-ized input signal
		double xnL = inputFrame[0];
		double xnR = inputChannels > 1 ? inputFrame[1] : 0.0;
		double monoXn = double(1.0 / inputChannels)*xnL + double(1.0 / inputChannels)*xnR;

		// --- pre delay output
		double preDelayOut = preDelay.processAudioSample(monoXn);

		// --- input to first branch = preDalay + globFB
		double input = preDelayOut + fb;
		for (int i = 0; i < NUM_BRANCHES; i++)
		{
			double apfOut = branchNestedAPFs[i].processAudioSample(input);
			double lpfOut = branchLPFs[i].processAudioSample(apfOut);
			double delayOut = parameters.kRT*branchDelays[i].processAudioSample(lpfOut);
			input = delayOut + preDelayOut;
		}
		// --- gather outputs
		/*
		There are 25 prime numbers between 1 and 100.
		They are 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41,
		43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, and 97

		we want 16 of them: 23, 29, 31, 37, 41,
		43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, and 97
		*/

		double weight = 0.707;

		double outL= 0.0;
		outL += weight*branchDelays[0].readDelayAtPercentage(23.0);
		outL -= weight*branchDelays[1].readDelayAtPercentage(41.0);
		outL += weight*branchDelays[2].readDelayAtPercentage(59.0);
		outL -= weight*branchDelays[3].readDelayAtPercentage(73.0);

		double outR = 0.0;
		outR -= weight*branchDelays[0].readDelayAtPercentage(29.0);
		outR += weight*branchDelays[1].readDelayAtPercentage(43.0);
		outR -= weight*branchDelays[2].readDelayAtPercentage(61.0);
		outR += weight*branchDelays[3].readDelayAtPercentage(79.0);

		if (parameters.density == reverbDensity::kThick)
		{
			outL += weight*branchDelays[0].readDelayAtPercentage(31.0);
			outL -= weight*branchDelays[1].readDelayAtPercentage(47.0);
			outL += weight*branchDelays[2].readDelayAtPercentage(67.0);
			outL -= weight*branchDelays[3].readDelayAtPercentage(83.0);

			outR -= weight*branchDelays[0].readDelayAtPercentage(37.0);
			outR += weight*branchDelays[1].readDelayAtPercentage(53.0);
			outR -= weight*branchDelays[2].readDelayAtPercentage(71.0);
			outR += weight*branchDelays[3].readDelayAtPercentage(89.0);
		}

		// ---  filter
		double tankOutL = shelvingFilters[0].processAudioSample(outL);
		double tankOutR = shelvingFilters[1].processAudioSample(outR);

		// --- sum with dry
		double dry = pow(10.0, parameters.dryLevel_dB / 20.0);
		double wet = pow(10.0, parameters.wetLevel_dB / 20.0);

		if (outputChannels == 1)
			outputFrame[0] = dry*xnL + wet*(0.5*tankOutL + 0.5*tankOutR);
		else
		{
			outputFrame[0] = dry*xnL + wet*tankOutL;
			outputFrame[1] = dry*xnR + wet*tankOutR;
		}

		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return ReverbTankParameters custom data structure
	*/
	ReverbTankParameters getParameters() { return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param ReverbTankParameters custom data structure
	*/
	void setParameters(const ReverbTankParameters& params)
	{
		// --- do the updates here, the sub-components will only update themselves if
		//     their parameters changed, so we let those object handle that chore
		TwoBandShelvingFilterParameters filterParams = shelvingFilters[0].getParameters();
		filterParams.highShelf_fc = params.highShelf_fc;
		filterParams.highShelfBoostCut_dB = params.highShelfBoostCut_dB;
		filterParams.lowShelf_fc = params.lowShelf_fc;
		filterParams.lowShelfBoostCut_dB = params.lowShelfBoostCut_dB;

		// --- copy to both channels
		shelvingFilters[0].setParameters(filterParams);
		shelvingFilters[1].setParameters(filterParams);

		SimpleLPFParameters  lpfParams = branchLPFs[0].getParameters();
		lpfParams.g = params.lpf_g;

		for (int i = 0; i < NUM_BRANCHES; i++)
		{
			branchLPFs[i].setParameters(lpfParams);
		}

		// --- update pre delay
		SimpleDelayParameters delayParams = preDelay.getParameters();
		delayParams.delayTime_mSec = params.preDelayTime_mSec;
		preDelay.setParameters(delayParams);

		// --- set apf and delay parameters
		int m = 0;
		NestedDelayAPFParameters apfParams = branchNestedAPFs[0].getParameters();
		delayParams = branchDelays[0].getParameters();

		// --- global max Delay times
		double globalAPFMaxDelay = (parameters.apfDelayWeight_Pct / 100.0)*parameters.apfDelayMax_mSec;
		double globalFixedMaxDelay = (parameters.fixeDelayWeight_Pct / 100.0)*parameters.fixeDelayMax_mSec;

		// --- lfo
		apfParams.enableLFO = true;
		apfParams.lfoMaxModulation_mSec = 0.3;
		apfParams.lfoDepth = 1.0;

		for (int i = 0; i < NUM_BRANCHES; i++)
		{
			// --- setup APFs
			apfParams.outerAPFdelayTime_mSec = globalAPFMaxDelay*apfDelayWeight[m++];
			apfParams.innerAPFdelayTime_mSec = globalAPFMaxDelay*apfDelayWeight[m++];
			apfParams.innerAPF_g = -0.5;
			apfParams.outerAPF_g = 0.5;
			if (i == 0)
				apfParams.lfoRate_Hz = 0.15;
			else if (i == 1)
				apfParams.lfoRate_Hz = 0.33;
			else if (i == 2)
				apfParams.lfoRate_Hz = 0.57;
			else if (i == 3)
				apfParams.lfoRate_Hz = 0.73;

			branchNestedAPFs[i].setParameters(apfParams);

			// --- fixedDelayWeight
			delayParams.delayTime_mSec = globalFixedMaxDelay*fixedDelayWeight[i];
			branchDelays[i].setParameters(delayParams);
		}

		// --- save our copy
		parameters = params;
	}


private:
	ReverbTankParameters parameters;				///< object parameters

	SimpleDelay  preDelay;							///< pre delay object
	SimpleDelay  branchDelays[NUM_BRANCHES];		///< branch delay objects
	NestedDelayAPF branchNestedAPFs[NUM_BRANCHES];	///< nested APFs for each branch
	SimpleLPF  branchLPFs[NUM_BRANCHES];			///< LPFs in each branch

	TwoBandShelvingFilter shelvingFilters[NUM_CHANNELS]; ///< shelving filters 0 = left; 1 = right

	// --- weighting values to make various and low-correlated APF delay values easily
	double apfDelayWeight[NUM_BRANCHES * 2] = { 0.317, 0.873, 0.477, 0.291, 0.993, 0.757, 0.179, 0.575 };///< weighting values to make various and low-correlated APF delay values easily
	double fixedDelayWeight[NUM_BRANCHES] = { 1.0, 0.873, 0.707, 0.667 };	///< weighting values to make various and fixed delay values easily
	double sampleRate = 0.0;	///< current sample rate
};


/**
\class PeakLimiter
\ingroup FX-Objects
\brief
The PeakLimiter object implements a simple peak limiter; it is really a simplified and hard-wired
versio of the DynamicsProcessor

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- setThreshold_dB(double _threshold_dB) to adjust the limiter threshold
- setMakeUpGain_dB(double _makeUpGain_dB) to adjust the makeup gain

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class PeakLimiter : public IAudioSignalProcessor
{
public:
	PeakLimiter() { setThreshold_dB(-3.0); }
	~PeakLimiter() {}

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- init; true = analog time-constant
		detector.setSampleRate(_sampleRate);

		AudioDetectorParameters detectorParams = detector.getParameters();
		detectorParams.detect_dB = true;
		detectorParams.attackTime_mSec = 5.0;
		detectorParams.releaseTime_mSec = 25.0;
		detectorParams.clampToUnityMax = false;
		detectorParams.detectMode = ENVELOPE_DETECT_MODE_PEAK;
		detector.setParameters(detectorParams);

		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process audio: implement hard limiter */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		return dB2Raw(makeUpGain_dB)*xn*computeGain(detector.processAudioSample(xn));
	}

	/** compute the gain reductino value based on detected value in dB */
	double computeGain(double detect_dB)
	{
		double output_dB = 0.0;

		// --- defaults - you can change these here
		bool softknee = true;
		double kneeWidth_dB = 10.0;

		// --- hard knee
		if (!softknee)
		{
			// --- below threshold, unity
			if (detect_dB <= threshold_dB)
				output_dB = detect_dB;
			// --- above threshold, compress
			else
				output_dB = threshold_dB;
		}
		else
		{
			// --- calc gain with knee
			// --- left side of knee, outside of width, unity gain zone
			if (2.0*(detect_dB - threshold_dB) < -kneeWidth_dB)
				output_dB = detect_dB;
			// --- inside the knee,
			else if (2.0*(fabs(detect_dB - threshold_dB)) <= kneeWidth_dB)
				output_dB = detect_dB - pow((detect_dB - threshold_dB + (kneeWidth_dB / 2.0)), 2.0) / (2.0*kneeWidth_dB);
			// --- right of knee, compression zone
			else if (2.0*(detect_dB - threshold_dB) > kneeWidth_dB)
				output_dB = threshold_dB;
		}

		// --- convert difference between threshold and detected to raw
		return  pow(10.0, (output_dB - detect_dB) / 20.0);
	}

	/** adjust threshold in dB */
	void setThreshold_dB(double _threshold_dB) { threshold_dB = _threshold_dB; }

	/** adjust makeup gain in dB*/
	void setMakeUpGain_dB(double _makeUpGain_dB) { makeUpGain_dB = _makeUpGain_dB; }

protected:
	AudioDetector detector;		///< the detector object
	double threshold_dB = 0.0;	///< stored threshold (dB)
	double makeUpGain_dB = 0.0;	///< stored makeup gain (dB)
};


/**
\enum vaFilterAlgorithm
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the virtual analog filter algorithm

- enum class vaFilterAlgorithm { kLPF1, kHPF1, kAPF1, kSVF_LP, kSVF_HP, kSVF_BP, kSVF_BS };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class vaFilterAlgorithm {
	kLPF1, kHPF1, kAPF1, kSVF_LP, kSVF_HP, kSVF_BP, kSVF_BS
}; // --- you will add more here...


/**
\struct ZVAFilterParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the ZVAFilter object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct ZVAFilterParameters
{
	ZVAFilterParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	ZVAFilterParameters& operator=(const ZVAFilterParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		filterAlgorithm = params.filterAlgorithm;
		fc = params.fc;
		Q = params.Q;
		filterOutputGain_dB = params.filterOutputGain_dB;
		enableGainComp = params.enableGainComp;
		matchAnalogNyquistLPF = params.matchAnalogNyquistLPF;
		selfOscillate = params.selfOscillate;
		enableNLP = params.enableNLP;
		return *this;
	}

	// --- individual parameters
	vaFilterAlgorithm filterAlgorithm = vaFilterAlgorithm::kSVF_LP;	///< va filter algorithm
	double fc = 1000.0;						///< va filter fc
	double Q = 0.707;						///< va filter Q
	double filterOutputGain_dB = 0.0;		///< va filter gain (normally unused)
	bool enableGainComp = false;			///< enable gain compensation (see book)
	bool matchAnalogNyquistLPF = false;		///< match analog gain at Nyquist
	bool selfOscillate = false;				///< enable selfOscillation
	bool enableNLP = false;					///< enable non linear processing (use oversampling for best results)
};


/**
\class ZVAFilter
\ingroup FX-Objects
\brief
The ZVAFilter object implements multpile Zavalishin VA Filters.
Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use BiquadParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class ZVAFilter : public IAudioSignalProcessor
{
public:
	ZVAFilter() {}		/* C-TOR */
	~ZVAFilter() {}		/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		integrator_z[0] = 0.0;
		integrator_z[1] = 0.0;

		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return ZVAFilterParameters custom data structure
	*/
	ZVAFilterParameters getParameters()
	{
		return zvaFilterParameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param ZVAFilterParameters custom data structure
	*/
	void setParameters(const ZVAFilterParameters& params)
	{
		if (params.fc != zvaFilterParameters.fc ||
			params.Q != zvaFilterParameters.Q ||
			params.selfOscillate != zvaFilterParameters.selfOscillate ||
			params.matchAnalogNyquistLPF != zvaFilterParameters.matchAnalogNyquistLPF)
		{
				zvaFilterParameters = params;
				calculateFilterCoeffs();
		}
		else
			zvaFilterParameters = params;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the VA filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- with gain comp enabled, we reduce the input by
		//     half the gain in dB at resonant peak
		//     NOTE: you can change that logic here!
		vaFilterAlgorithm filterAlgorithm = zvaFilterParameters.filterAlgorithm;
		bool matchAnalogNyquistLPF = zvaFilterParameters.matchAnalogNyquistLPF;

		if (zvaFilterParameters.enableGainComp)
		{
			double peak_dB = dBPeakGainFor_Q(zvaFilterParameters.Q);
			if (peak_dB > 0.0)
			{
				double halfPeak_dBGain = dB2Raw(-peak_dB / 2.0);
				xn *= halfPeak_dBGain;
			}
		}

		// --- for 1st order filters:
		if (filterAlgorithm == vaFilterAlgorithm::kLPF1 ||
			filterAlgorithm == vaFilterAlgorithm::kHPF1 ||
			filterAlgorithm == vaFilterAlgorithm::kAPF1)
		{
			// --- create vn node
			double vn = (xn - integrator_z[0])*alpha;

			// --- form LP output
			double lpf = ((xn - integrator_z[0])*alpha) + integrator_z[0];

			double sn = integrator_z[0];

			// --- update memory
			integrator_z[0] = vn + lpf;

			// --- form the HPF = INPUT = LPF
			double hpf = xn - lpf;

			// --- form the APF = LPF - HPF
			double apf = lpf - hpf;

			// --- set the outputs
			if (filterAlgorithm == vaFilterAlgorithm::kLPF1)
			{
				// --- this is a very close match as-is at Nyquist!
				if (matchAnalogNyquistLPF)
					return lpf + alpha*hpf;
				else
					return lpf;
			}
			else if (filterAlgorithm == vaFilterAlgorithm::kHPF1)
				return hpf;
			else if (filterAlgorithm == vaFilterAlgorithm::kAPF1)
				return apf;

			// --- unknown filter
			return xn;
		}

		// --- form the HP output first
		double hpf = alpha0*(xn - rho*integrator_z[0] - integrator_z[1]);

		// --- BPF Out
		double bpf = alpha*hpf + integrator_z[0];
		if (zvaFilterParameters.enableNLP)
			bpf = softClipWaveShaper(bpf, 1.0);

		// --- LPF Out
		double lpf = alpha*bpf + integrator_z[1];

		// --- BSF Out
		double bsf = hpf + lpf;

		// --- finite gain at Nyquist; slight error at VHF
		double sn = integrator_z[0];

		// update memory
		integrator_z[0] = alpha*hpf + bpf;
		integrator_z[1] = alpha*bpf + lpf;

		double filterOutputGain = pow(10.0, zvaFilterParameters.filterOutputGain_dB / 20.0);

		// return our selected type
		if (filterAlgorithm == vaFilterAlgorithm::kSVF_LP)
		{
			if (matchAnalogNyquistLPF)
				lpf += analogMatchSigma*(sn);
			return filterOutputGain*lpf;
		}
		else if (filterAlgorithm == vaFilterAlgorithm::kSVF_HP)
			return filterOutputGain*hpf;
		else if (filterAlgorithm == vaFilterAlgorithm::kSVF_BP)
			return filterOutputGain*bpf;
		else if (filterAlgorithm == vaFilterAlgorithm::kSVF_BS)
			return filterOutputGain*bsf;

		// --- unknown filter
		return filterOutputGain*lpf;
	}

	/** recalculate the filter coefficients*/
	void calculateFilterCoeffs()
	{
		double fc = zvaFilterParameters.fc;
		double Q = zvaFilterParameters.Q;
		vaFilterAlgorithm filterAlgorithm = zvaFilterParameters.filterAlgorithm;

		// --- normal Zavalishin SVF calculations here
		//     prewarp the cutoff- these are bilinear-transform filters
		double wd = kTwoPi*fc;
		double T = 1.0 / sampleRate;
		double wa = (2.0 / T)*tan(wd*T / 2.0);
		double g = wa*T / 2.0;

		// --- for 1st order filters:
		if (filterAlgorithm == vaFilterAlgorithm::kLPF1 ||
			filterAlgorithm == vaFilterAlgorithm::kHPF1 ||
			filterAlgorithm == vaFilterAlgorithm::kAPF1)
		{
			// --- calculate alpha
			alpha = g / (1.0 + g);
		}
		else // state variable variety
		{
			// --- note R is the traditional analog damping factor zeta
			double R = zvaFilterParameters.selfOscillate ? 0.0 : 1.0 / (2.0*Q);
			alpha0 = 1.0 / (1.0 + 2.0*R*g + g*g);
			alpha = g;
			rho = 2.0*R + g;

			// --- sigma for analog matching version
			double f_o = (sampleRate / 2.0) / fc;
			analogMatchSigma = 1.0 / (alpha*f_o*f_o);
		}
	}

	/** set beta value, for filters that aggregate 1st order VA sections*/
	void setBeta(double _beta) { beta = _beta; }

	/** get beta value,not used in book projects; for future use*/
	double getBeta() { return beta; }

protected:
	ZVAFilterParameters zvaFilterParameters;	///< object parameters
	double sampleRate = 44100.0;				///< current sample rate

	// --- state storage
	double integrator_z[2];						///< state variables

	// --- filter coefficients
	double alpha0 = 0.0;		///< input scalar, correct delay-free loop
	double alpha = 0.0;			///< alpha is (wcT/2)
	double rho = 0.0;			///< p = 2R + g (feedback)

	double beta = 0.0;			///< beta value, not used

	// --- for analog Nyquist matching
	double analogMatchSigma = 0.0; ///< analog matching Sigma value (see book)

};

/**
\struct EnvelopeFollowerParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the EnvelopeFollower object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct EnvelopeFollowerParameters
{
	EnvelopeFollowerParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	EnvelopeFollowerParameters& operator=(const EnvelopeFollowerParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		fc = params.fc;
		Q = params.Q;
		attackTime_mSec = params.attackTime_mSec;
		releaseTime_mSec = params.releaseTime_mSec;
		threshold_dB = params.threshold_dB;
		sensitivity = params.sensitivity;

		return *this;
	}

	// --- individual parameters
	double fc = 0.0;				///< filter fc
	double Q = 0.707;				///< filter Q
	double attackTime_mSec = 10.0;	///< detector attack time
	double releaseTime_mSec = 10.0;	///< detector release time
	double threshold_dB = 0.0;		///< detector threshold in dB
	double sensitivity = 1.0;		///< detector sensitivity
};

/**
\class EnvelopeFollower
\ingroup FX-Objects
\brief
The EnvelopeFollower object implements a traditional envelope follower effect modulating a LPR fc value
using the strength of the detected input.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use EnvelopeFollowerParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class EnvelopeFollower : public IAudioSignalProcessor
{
public:
	EnvelopeFollower() {
		// --- setup the filter
		ZVAFilterParameters filterParams;
		filterParams.filterAlgorithm = vaFilterAlgorithm::kSVF_LP;
		filterParams.fc = 1000.0;
		filterParams.enableGainComp = true;
		filterParams.enableNLP = true;
		filterParams.matchAnalogNyquistLPF = true;
		filter.setParameters(filterParams);

		// --- setup the detector
		AudioDetectorParameters adParams;
		adParams.attackTime_mSec = -1.0;
		adParams.releaseTime_mSec = -1.0;
		adParams.detectMode = TLD_AUDIO_DETECT_MODE_RMS;
		adParams.detect_dB = true;
		adParams.clampToUnityMax = false;
		detector.setParameters(adParams);

	}		/* C-TOR */
	~EnvelopeFollower() {}		/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		filter.reset(_sampleRate);
		detector.reset(_sampleRate);
		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return EnvelopeFollowerParameters custom data structure
	*/
	EnvelopeFollowerParameters getParameters() { return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param EnvelopeFollowerParameters custom data structure
	*/
	void setParameters(const EnvelopeFollowerParameters& params)
	{
		ZVAFilterParameters filterParams = filter.getParameters();
		AudioDetectorParameters adParams = detector.getParameters();

		if (params.fc != parameters.fc || params.Q != parameters.Q)
		{
			filterParams.fc = params.fc;
			filterParams.Q = params.Q;
			filter.setParameters(filterParams);
		}
		if (params.attackTime_mSec != parameters.attackTime_mSec ||
			params.releaseTime_mSec != parameters.releaseTime_mSec)
		{
			adParams.attackTime_mSec = params.attackTime_mSec;
			adParams.releaseTime_mSec = params.releaseTime_mSec;
			detector.setParameters(adParams);
		}

		// --- save
		parameters = params;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the envelope follower to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- calc threshold
		double threshValue = pow(10.0, parameters.threshold_dB / 20.0);

		// --- detect the signal
		double detect_dB = detector.processAudioSample(xn);
		double detectValue = pow(10.0, detect_dB / 20.0);
		double deltaValue = detectValue - threshValue;

		ZVAFilterParameters filterParams = filter.getParameters();
		filterParams.fc = parameters.fc;

		// --- if above the threshold, modulate the filter fc
		if (deltaValue > 0.0)// || delta_dB > 0.0)
		{
			// --- fc Computer
			double modulatorValue = 0.0;

			// --- best results are with linear values when detector is in dB mode
			modulatorValue = (deltaValue * parameters.sensitivity);

			// --- calculate modulated frequency
			filterParams.fc = doUnipolarModulationFromMin(modulatorValue, parameters.fc, kMaxFilterFrequency);
		}

		// --- update with new modulated frequency
		filter.setParameters(filterParams);

		// --- perform the filtering operation
		return filter.processAudioSample(xn);
	}

protected:
	EnvelopeFollowerParameters parameters; ///< object parameters

	// --- 1 filter and 1 detector
	ZVAFilter filter;		///< filter to modulate
	AudioDetector detector; ///< detector to track input signal
};

/**
\enum distortionModel
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the waveshaper model for the Triode objects

- enum class distortionModel { kSoftClip, kArcTan, kFuzzAsym };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class distortionModel { kSoftClip, kArcTan, kFuzzAsym };

/**
\struct TriodeClassAParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the TriodeClassA object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct TriodeClassAParameters
{
	TriodeClassAParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	TriodeClassAParameters& operator=(const TriodeClassAParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		waveshaper = params.waveshaper;
		saturation = params.saturation;
		asymmetry = params.asymmetry;
		outputGain = params.outputGain;

		invertOutput = params.invertOutput;
		enableHPF = params.enableHPF;
		enableLSF = params.enableLSF;

		hpf_Fc = params.hpf_Fc;
		lsf_Fshelf = params.lsf_Fshelf;
		lsf_BoostCut_dB = params.lsf_BoostCut_dB;

		return *this;
	}

	// --- individual parameters
	distortionModel waveshaper = distortionModel::kSoftClip; ///< waveshaper

	double saturation = 1.0;	///< saturation level
	double asymmetry = 0.0;		///< asymmetry level
	double outputGain = 1.0;	///< outputGain level

	bool invertOutput = true;	///< invertOutput - triodes invert output
	bool enableHPF = true;		///< HPF simulates DC blocking cap on output
	bool enableLSF = false;		///< LSF simulates shelf due to cathode self biasing

	double hpf_Fc = 1.0;		///< fc of DC blocking cap
	double lsf_Fshelf = 80.0;	///< shelf fc from self bias cap
	double lsf_BoostCut_dB = 0.0;///< boost/cut due to cathode self biasing
};

/**
\class TriodeClassA
\ingroup FX-Objects
\brief
The TriodeClassA object simulates a triode in class A configuration. This is a very simple and basic simulation
and a starting point for other designs; it is not intended to be a full-fledged triode simulator.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use TriodeClassAParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class TriodeClassA : public IAudioSignalProcessor
{
public:
	TriodeClassA() {
		AudioFilterParameters params;
		params.algorithm = filterAlgorithm::kHPF1;
		params.fc = parameters.hpf_Fc;
		outputHPF.setParameters(params);

		params.algorithm = filterAlgorithm::kLowShelf;
		params.fc = parameters.lsf_Fshelf;
		params.boostCut_dB = parameters.lsf_BoostCut_dB;
		outputLSF.setParameters(params);
	}		/* C-TOR */
	~TriodeClassA() {}		/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		outputHPF.reset(_sampleRate);
		outputLSF.reset(_sampleRate);

		// ---
		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return TriodeClassAParameters custom data structure
	*/
	TriodeClassAParameters getParameters() { return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param TriodeClassAParameters custom data structure
	*/
	void setParameters(const TriodeClassAParameters& params)
	{
		parameters = params;

		AudioFilterParameters filterParams;
		filterParams.algorithm = filterAlgorithm::kHPF1;
		filterParams.fc = parameters.hpf_Fc;
		outputHPF.setParameters(filterParams);

		filterParams.algorithm = filterAlgorithm::kLowShelf;
		filterParams.fc = parameters.lsf_Fshelf;
		filterParams.boostCut_dB = parameters.lsf_BoostCut_dB;
		outputLSF.setParameters(filterParams);
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** do the triode simulation to process one input to one output*/
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- perform waveshaping
		double output = 0.0;

		if (parameters.waveshaper == distortionModel::kSoftClip)
			output = softClipWaveShaper(xn, parameters.saturation);
		else if (parameters.waveshaper == distortionModel::kArcTan)
			output = atanWaveShaper(xn, parameters.saturation);
		else if (parameters.waveshaper == distortionModel::kFuzzAsym)
			output = fuzzExp1WaveShaper(xn, parameters.saturation, parameters.asymmetry);

		// --- inversion, normal for plate of class A triode
		if (parameters.invertOutput)
			output *= -1.0;

		// --- Output (plate) capacitor = HPF, remove DC offset
		if (parameters.enableHPF)
			output = outputHPF.processAudioSample(output);

		// --- if cathode resistor bypass, will create low shelf
		if (parameters.enableLSF)
			output = outputLSF.processAudioSample(output);

		// --- final resistor divider/potentiometer
		output *= parameters.outputGain;

		return output;
	}

protected:
	TriodeClassAParameters parameters;	///< object parameters
	AudioFilter outputHPF;				///< HPF to simulate output DC blocking cap
	AudioFilter outputLSF;				///< LSF to simulate shelf caused by cathode self-biasing cap
};

const unsigned int NUM_TUBES = 4;

/**
\struct ClassATubePreParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the ClassATubePre object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct ClassATubePreParameters
{
	ClassATubePreParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	ClassATubePreParameters& operator=(const ClassATubePreParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		inputLevel_dB = params.inputLevel_dB;
		saturation = params.saturation;
		asymmetry = params.asymmetry;
		outputLevel_dB = params.outputLevel_dB;

		lowShelf_fc = params.lowShelf_fc;
		lowShelfBoostCut_dB = params.lowShelfBoostCut_dB;
		highShelf_fc = params.highShelf_fc;
		highShelfBoostCut_dB = params.highShelfBoostCut_dB;

		return *this;
	}

	// --- individual parameters
	double inputLevel_dB = 0.0;		///< input level in dB
	double saturation = 0.0;		///< input level in dB
	double asymmetry = 0.0;			///< input level in dB
	double outputLevel_dB = 0.0;	///< input level in dB

	// --- shelving filter params
	double lowShelf_fc = 0.0;			///< LSF shelf frequency
	double lowShelfBoostCut_dB = 0.0;	///< LSF shelf gain/cut
	double highShelf_fc = 0.0;			///< HSF shelf frequency
	double highShelfBoostCut_dB = 0.0;	///< HSF shelf frequency

};

/**
\class ClassATubePre
\ingroup FX-Objects
\brief
The ClassATubePre object implements a simple cascade of four (4) triode tube models.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use ClassATubePreParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class ClassATubePre : public IAudioSignalProcessor
{
public:
	ClassATubePre() {}		/* C-TOR */
	~ClassATubePre() {}		/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		TriodeClassAParameters tubeParams = triodes[0].getParameters();
		tubeParams.invertOutput = true;
		tubeParams.enableHPF = true; // remove DC offsets
		tubeParams.outputGain = 1.0;
		tubeParams.saturation = 1.0;
		tubeParams.asymmetry = 0.0;
		tubeParams.enableLSF = true;
		tubeParams.lsf_Fshelf = 88.0;
		tubeParams.lsf_BoostCut_dB = -12.0;
		tubeParams.waveshaper = distortionModel::kFuzzAsym;

		for (int i = 0; i < NUM_TUBES; i++)
		{
			triodes[i].reset(_sampleRate);
			triodes[i].setParameters(tubeParams);
		}

		shelvingFilter.reset(_sampleRate);

		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return ClassATubePreParameters custom data structure
	*/
	ClassATubePreParameters getParameters() { return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param ClassATubePreParameters custom data structure
	*/
	void setParameters(const ClassATubePreParameters& params)
	{
		// --- check for re-calc
		if (params.inputLevel_dB != parameters.inputLevel_dB)
			inputLevel = pow(10.0, params.inputLevel_dB / 20.0);
		if (params.outputLevel_dB != parameters.outputLevel_dB)
			outputLevel = pow(10.0, params.outputLevel_dB / 20.0);

		// --- store
		parameters = params;

		// --- shelving filter update
		TwoBandShelvingFilterParameters sfParams = shelvingFilter.getParameters();
		sfParams.lowShelf_fc = parameters.lowShelf_fc;
		sfParams.lowShelfBoostCut_dB = parameters.lowShelfBoostCut_dB;
		sfParams.highShelf_fc = parameters.highShelf_fc;
		sfParams.highShelfBoostCut_dB = parameters.highShelfBoostCut_dB;
		shelvingFilter.setParameters(sfParams);

		// --- triode updates
		TriodeClassAParameters tubeParams = triodes[0].getParameters();
		tubeParams.saturation = parameters.saturation;
		tubeParams.asymmetry = parameters.asymmetry;

		for (int i = 0; i < NUM_TUBES; i++)
			triodes[i].setParameters(tubeParams);
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process the input through the four tube models in series */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		double output1 = triodes[0].processAudioSample(xn*inputLevel);
		double output2 = triodes[1].processAudioSample(output1);
		double output3 = triodes[2].processAudioSample(output2);

		// --- filter stage is between 3 and 4
		double outputEQ = shelvingFilter.processAudioSample(output3);
		double output4 = triodes[3].processAudioSample(outputEQ);

		return output4*outputLevel;
	}

protected:
	ClassATubePreParameters parameters;		///< object parameters
	TriodeClassA triodes[NUM_TUBES];		///< array of triode tube objects
	TwoBandShelvingFilter shelvingFilter;	///< shelving filters

	double inputLevel = 1.0;	///< input level (not in dB)
	double outputLevel = 1.0;	///< output level (not in dB)
};


/**
\struct BitCrusherParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the BitCrusher object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct BitCrusherParameters
{
	BitCrusherParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	BitCrusherParameters& operator=(const BitCrusherParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		quantizedBitDepth = params.quantizedBitDepth;

		return *this;
	}

	double quantizedBitDepth = 4.0; ///< bid depth of quantizer
};

/**
\class BitCrusher
\ingroup FX-Objects
\brief
The BitCrusher object implements a quantizing bitcrusher algorithm.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use BitCrusherParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class BitCrusher : public IAudioSignalProcessor
{
public:
	BitCrusher() {}		/* C-TOR */
	~BitCrusher() {}		/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate){ return true; }

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return BitCrusherParameters custom data structure
	*/
	BitCrusherParameters getParameters() { return parameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param BitCrusherParameters custom data structure
	*/
	void setParameters(const BitCrusherParameters& params)
	{
		// --- calculate and store
		if (params.quantizedBitDepth != parameters.quantizedBitDepth)
			QL = 2.0 / (pow(2.0, params.quantizedBitDepth) - 1.0);

		parameters = params;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** perform the bitcrushing operation (see FX book for back story and details) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		return QL*(int(xn / QL));
	}

protected:
	BitCrusherParameters parameters; ///< object parameters
	double QL = 1.0;				 ///< the quantization level
};


// ------------------------------------------------------------------ //
// --- WDF LIBRARY -------------------------------------------------- //
// ------------------------------------------------------------------ //

/**
\class IComponentAdaptor
\ingroup Interfaces
\brief
Use this interface for objects in the WDF Ladder Filter library; see book for more information.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class IComponentAdaptor
{
public:
	/** initialize with source resistor R1 */
	virtual void initialize(double _R1) {}

	/** initialize all downstream adaptors in the chain */
	virtual void initializeAdaptorChain() {}

	/** set input value into component port  */
	virtual void setInput(double _in) {}

	/** get output value from component port  */
	virtual double getOutput() { return 0.0; }

	// --- for adaptors
	/** ADAPTOR: set input port 1  */
	virtual void setInput1(double _in1) = 0;

	/** ADAPTOR: set input port 2  */
	virtual void setInput2(double _in2) = 0;

	/** ADAPTOR: set input port 3 */
	virtual void setInput3(double _in3) = 0;

	/** ADAPTOR: get output port 1 value */
	virtual double getOutput1() = 0;

	/** ADAPTOR: get output port 2 value */
	virtual double getOutput2() = 0;

	/** ADAPTOR: get output port 3 value */
	virtual double getOutput3() = 0;

	/** reset the object with new sample rate */
	virtual void reset(double _sampleRate) {}

	/** get the commponent resistance from the attached object at Port3 */
	virtual double getComponentResistance() { return 0.0; }

	/** get the commponent conductance from the attached object at Port3 */
	virtual double getComponentConductance() { return 0.0; }

	/** update the commponent resistance at Port3 */
	virtual void updateComponentResistance() {}

	/** set an individual component value (may be R, L, or C */
	virtual void setComponentValue(double _componentValue) { }

	/** set LC combined values */
	virtual void setComponentValue_LC(double componentValue_L, double componentValue_C) { }

	/** set RL combined values */
	virtual void setComponentValue_RL(double componentValue_R, double componentValue_L) { }

	/** set RC combined values */
	virtual void setComponentValue_RC(double componentValue_R, double componentValue_C) { }

	/** get a component value */
	virtual double getComponentValue() { return 0.0; }
};

// ------------------------------------------------------------------ //
// --- WDF COMPONENTS & COMMBO COMPONENTS --------------------------- //
// ------------------------------------------------------------------ //
/**
\class WdfResistor
\ingroup WDF-Objects
\brief
The WdfResistor object implements the reflection coefficient and signal flow through
a WDF simulated resistor.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfResistor : public IComponentAdaptor
{
public:
	WdfResistor(double _componentValue) { componentValue = _componentValue; }
	WdfResistor() { }
	virtual ~WdfResistor() {}

	/** set sample rate and update component */
	void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		updateComponentResistance();
	}

	/** get component's value as a resistance */
	virtual double getComponentResistance() { return componentResistance; }

	/** get component's value as a conducatance (or admittance) */
	virtual double getComponentConductance() { return 1.0 / componentResistance; }

	/** get the component value */
	virtual double getComponentValue() { return componentValue; }

	/** set the component value */
	virtual void setComponentValue(double _componentValue)
	{
		componentValue = _componentValue;
		updateComponentResistance();
	}

	/** change the resistance of component */
	virtual void updateComponentResistance() { componentResistance = componentValue; }

	/** reset the component; clear registers */
	virtual void reset(double _sampleRate) { setSampleRate(_sampleRate);  zRegister = 0.0; }

	/** set input value into component; NOTE: resistor is dead-end energy sink so this function does nothing */
	virtual void setInput(double in) {}

	/** get output value; NOTE: a WDF resistor produces no reflected output */
	virtual double getOutput() { return 0.0; }

	/** get output1 value; only one resistor output (not used) */
	virtual double getOutput1() { return  getOutput(); }

	/** get output2 value; only one resistor output (not used) */
	virtual double getOutput2() { return  getOutput(); }

	/** get output3 value; only one resistor output (not used) */
	virtual double getOutput3() { return  getOutput(); }

	/** set input1 value; not used for components */
	virtual void setInput1(double _in1) {}

	/** set input2 value; not used for components */
	virtual void setInput2(double _in2) {}

	/** set input3 value; not used for components */
	virtual void setInput3(double _in3) {}

protected:
	double zRegister = 0.0;			///< storage register (not used with resistor)
	double componentValue = 0.0;	///< component value in electronic form (ohm, farad, henry)
	double componentResistance = 0.0;///< simulated resistance
	double sampleRate = 0.0;		///< sample rate
};


/**
\class WdfCapacitor
\ingroup WDF-Objects
\brief
The WdfCapacitor object implements the reflection coefficient and signal flow through
a WDF simulated capacitor.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfCapacitor : public IComponentAdaptor
{
public:
	WdfCapacitor(double _componentValue) { componentValue = _componentValue; }
	WdfCapacitor() { }
	virtual ~WdfCapacitor() {}

	/** set sample rate and update component */
	void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		updateComponentResistance();
	}

	/** get component's value as a resistance */
	virtual double getComponentResistance() { return componentResistance; }

	/** get component's value as a conducatance (or admittance) */
	virtual double getComponentConductance() { return 1.0 / componentResistance; }

	/** get the component value */
	virtual double getComponentValue() { return componentValue; }

	/** set the component value */
	virtual void setComponentValue(double _componentValue)
	{
		componentValue = _componentValue;
		updateComponentResistance();
	}

	/** change the resistance of component */
	virtual void updateComponentResistance()
	{
		componentResistance = 1.0 / (2.0*componentValue*sampleRate);
	}

	/** reset the component; clear registers */
	virtual void reset(double _sampleRate) { setSampleRate(_sampleRate); zRegister = 0.0; }

	/** set input value into component; NOTE: capacitor sets value into register*/
	virtual void setInput(double in) { zRegister = in; }

	/** get output value; NOTE: capacitor produces reflected output */
	virtual double getOutput() { return zRegister; }	// z^-1

	/** get output1 value; only one capacitor output (not used) */
	virtual double getOutput1() { return  getOutput(); }

	/** get output2 value; only one capacitor output (not used) */
	virtual double getOutput2() { return  getOutput(); }

	/** get output3 value; only one capacitor output (not used) */
	virtual double getOutput3() { return  getOutput(); }

	/** set input1 value; not used for components */
	virtual void setInput1(double _in1) {}

	/** set input2 value; not used for components */
	virtual void setInput2(double _in2) {}

	/** set input3 value; not used for components */
	virtual void setInput3(double _in3) {}

protected:
	double zRegister = 0.0;			///< storage register (not used with resistor)
	double componentValue = 0.0;	///< component value in electronic form (ohm, farad, henry)
	double componentResistance = 0.0;///< simulated resistance
	double sampleRate = 0.0;		///< sample rate
};

/**
\class WdfInductor
\ingroup WDF-Objects
\brief
The WdfInductor object implements the reflection coefficient and signal flow through
a WDF simulated inductor.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfInductor : public IComponentAdaptor
{
public:
	WdfInductor(double _componentValue) { componentValue = _componentValue; }
	WdfInductor() { }
	virtual ~WdfInductor() {}

	/** set sample rate and update component */
	void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		updateComponentResistance();
	}

	/** get component's value as a resistance */
	virtual double getComponentResistance() { return componentResistance; }

	/** get component's value as a conducatance (or admittance) */
	virtual double getComponentConductance() { return 1.0 / componentResistance; }

	/** get the component value */
	virtual double getComponentValue() { return componentValue; }

	/** set the component value */
	virtual void setComponentValue(double _componentValue)
	{
		componentValue = _componentValue;
		updateComponentResistance();
	}

	/** change the resistance of component R(L) = 2Lfs */
	virtual void updateComponentResistance(){ componentResistance = 2.0*componentValue*sampleRate;}

	/** reset the component; clear registers */
	virtual void reset(double _sampleRate) { setSampleRate(_sampleRate); zRegister = 0.0; }

	/** set input value into component; NOTE: inductor sets value into storage register */
	virtual void setInput(double in) { zRegister = in; }

	/** get output value; NOTE: a WDF inductor produces reflected output that is inverted */
	virtual double getOutput() { return -zRegister; } // -z^-1

	/** get output1 value; only one resistor output (not used) */
	virtual double getOutput1() { return  getOutput(); }

	/** get output2 value; only one resistor output (not used) */
	virtual double getOutput2() { return  getOutput(); }

	/** get output3 value; only one resistor output (not used) */
	virtual double getOutput3() { return  getOutput(); }

	/** set input1 value; not used for components */
	virtual void setInput1(double _in1) {}

	/** set input2 value; not used for components */
	virtual void setInput2(double _in2) {}

	/** set input3 value; not used for components */
	virtual void setInput3(double _in3) {}

protected:
	double zRegister = 0.0;			///< storage register (not used with resistor)
	double componentValue = 0.0;	///< component value in electronic form (ohm, farad, henry)
	double componentResistance = 0.0;///< simulated resistance
	double sampleRate = 0.0;		///< sample rate
};


/**
\class WdfSeriesLC
\ingroup WDF-Objects
\brief
The WdfSeriesLC object implements the reflection coefficient and signal flow through
a WDF simulated series LC pair.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
SEE: p143 "Design of Wave Digital Filters" Psenicka, Ugalde, Romero M.
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfSeriesLC : public IComponentAdaptor
{
public:
	WdfSeriesLC() {}
	WdfSeriesLC(double _componentValue_L, double _componentValue_C)
	{
		componentValue_L = _componentValue_L;
		componentValue_C = _componentValue_C;
	}
	virtual ~WdfSeriesLC() {}

	/** set sample rate and update component */
	void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		updateComponentResistance();
	}

	/** get component's value as a resistance */
	virtual double getComponentResistance() { return componentResistance; }

	/** get component's value as a conducatance (or admittance) */
	virtual double getComponentConductance() { return 1.0 / componentResistance; }

	/** change the resistance of component; see FX book for details */
	virtual void updateComponentResistance()
	{
		RL = 2.0*componentValue_L*sampleRate;
		RC = 1.0 / (2.0*componentValue_C*sampleRate);
		componentResistance = RL + (1.0 / RC);
	}

	/** set both LC components at once */
	virtual void setComponentValue_LC(double _componentValue_L, double _componentValue_C)
	{
		componentValue_L = _componentValue_L;
		componentValue_C = _componentValue_C;
		updateComponentResistance();
	}

	/** set L component */
	virtual void setComponentValue_L(double _componentValue_L)
	{
		componentValue_L = _componentValue_L;
		updateComponentResistance();
	}

	/** set C component */
	virtual void setComponentValue_C(double _componentValue_C)
	{
		componentValue_C = _componentValue_C;
		updateComponentResistance();
	}

	/** get L component value */
	virtual double getComponentValue_L() { return componentValue_L; }

	/** get C component value */
	virtual double getComponentValue_C() { return componentValue_C; }

	/** reset the component; clear registers */
	virtual void reset(double _sampleRate) { setSampleRate(_sampleRate); zRegister_L = 0.0; zRegister_C = 0.0; }

	/** set input value into component; NOTE: K is calculated here */
	virtual void setInput(double in)
	{
		double YC = 1.0 / RC;
		double K = (1.0 - RL*YC) / (1.0 + RL*YC);
		double N1 = K*(in - zRegister_L);
		zRegister_L = N1 + zRegister_C;
		zRegister_C = in;
	}

	/** get output value; NOTE: utput is located in zReg_L */
	virtual double getOutput(){ return zRegister_L; }

	/** get output1 value; only one resistor output (not used) */
	virtual double getOutput1() { return  getOutput(); }

	/** get output2 value; only one resistor output (not used) */
	virtual double getOutput2() { return  getOutput(); }

	/** get output3 value; only one resistor output (not used) */
	virtual double getOutput3() { return  getOutput(); }

	/** set input1 value; not used for components */
	virtual void setInput1(double _in1) {}

	/** set input2 value; not used for components */
	virtual void setInput2(double _in2) {}

	/** set input3 value; not used for components */
	virtual void setInput3(double _in3) {}

protected:
	double zRegister_L = 0.0; ///< storage register for L
	double zRegister_C = 0.0; ///< storage register for C

	double componentValue_L = 0.0; ///< component value L
	double componentValue_C = 0.0; ///< component value C

	double RL = 0.0; ///< RL value
	double RC = 0.0; ///< RC value
	double componentResistance = 0.0; ///< equivalent resistance of pair of components
	double sampleRate = 0.0; ///< sample rate
};

/**
\class WdfParallelLC
\ingroup WDF-Objects
\brief
The WdfParallelLC object implements the reflection coefficient and signal flow through
a WDF simulated parallel LC pair.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
SEE: p143 "Design of Wave Digital Filters" Psenicka, Ugalde, Romero M.
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfParallelLC : public IComponentAdaptor
{
public:
	WdfParallelLC() {}
	WdfParallelLC(double _componentValue_L, double _componentValue_C)
	{
		componentValue_L = _componentValue_L;
		componentValue_C = _componentValue_C;
	}
	virtual ~WdfParallelLC() {}

	/** set sample rate and update component */
	void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		updateComponentResistance();
	}

	/** get component's value as a resistance */
	virtual double getComponentResistance() { return componentResistance; }

	/** get component's value as a conducatance (or admittance) */
	virtual double getComponentConductance() { return 1.0 / componentResistance; }

	/** change the resistance of component; see FX book for details */
	virtual void updateComponentResistance()
	{
		RL = 2.0*componentValue_L*sampleRate;
		RC = 1.0 / (2.0*componentValue_C*sampleRate);
		componentResistance = (RC + 1.0 / RL);
	}

	/** set both LC components at once */
	virtual void setComponentValue_LC(double _componentValue_L, double _componentValue_C)
	{
		componentValue_L = _componentValue_L;
		componentValue_C = _componentValue_C;
		updateComponentResistance();
	}

	/** set L component */
	virtual void setComponentValue_L(double _componentValue_L)
	{
		componentValue_L = _componentValue_L;
		updateComponentResistance();
	}

	/** set C component */
	virtual void setComponentValue_C(double _componentValue_C)
	{
		componentValue_C = _componentValue_C;
		updateComponentResistance();
	}

	/** get L component value */
	virtual double getComponentValue_L() { return componentValue_L; }

	/** get C component value */
	virtual double getComponentValue_C() { return componentValue_C; }

	/** reset the component; clear registers */
	virtual void reset(double _sampleRate) { setSampleRate(_sampleRate); zRegister_L = 0.0; zRegister_C = 0.0; }

	/** set input value into component; NOTE: K is calculated here */
	virtual void setInput(double in)
	{
		double YL = 1.0 / RL;
		double K = (YL*RC - 1.0) / (YL*RC + 1.0);
		double N1 = K*(in - zRegister_L);
		zRegister_L = N1 + zRegister_C;
		zRegister_C = in;
	}

	/** get output value; NOTE: output is located in -zReg_L */
	virtual double getOutput(){ return -zRegister_L; }

	/** get output1 value; only one resistor output (not used) */
	virtual double getOutput1() { return  getOutput(); }

	/** get output2 value; only one resistor output (not used) */
	virtual double getOutput2() { return  getOutput(); }

	/** get output3 value; only one resistor output (not used) */
	virtual double getOutput3() { return  getOutput(); }

	/** set input1 value; not used for components */
	virtual void setInput1(double _in1) {}

	/** set input2 value; not used for components */
	virtual void setInput2(double _in2) {}

	/** set input3 value; not used for components */
	virtual void setInput3(double _in3) {}

protected:
	double zRegister_L = 0.0; ///< storage register for L
	double zRegister_C = 0.0; ///< storage register for C

	double componentValue_L = 0.0; ///< component value L
	double componentValue_C = 0.0; ///< component value C

	double RL = 0.0; ///< RL value
	double RC = 0.0; ///< RC value
	double componentResistance = 0.0; ///< equivalent resistance of pair of components
	double sampleRate = 0.0; ///< sample rate
};


/**
\class WdfSeriesRL
\ingroup WDF-Objects
\brief
The WdfSeriesRL object implements the reflection coefficient and signal flow through
a WDF simulated series RL pair.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
SEE: p143 "Design of Wave Digital Filters" Psenicka, Ugalde, Romero M.
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfSeriesRL : public IComponentAdaptor
{
public:
	WdfSeriesRL() {}
	WdfSeriesRL(double _componentValue_R, double _componentValue_L)
	{
		componentValue_L = _componentValue_L;
		componentValue_R = _componentValue_R;
	}
	virtual ~WdfSeriesRL() {}

	/** set sample rate and update component */
	void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		updateComponentResistance();
	}

	/** get component's value as a resistance */
	virtual double getComponentResistance() { return componentResistance; }

	/** get component's value as a conducatance (or admittance) */
	virtual double getComponentConductance() { return 1.0 / componentResistance; }

	/** change the resistance of component; see FX book for details */
	virtual void updateComponentResistance()
	{
		RR = componentValue_R;
		RL = 2.0*componentValue_L*sampleRate;
		componentResistance = RR + RL;
		K = RR / componentResistance;
	}

	/** set both RL components at once */
	virtual void setComponentValue_RL(double _componentValue_R, double _componentValue_L)
	{
		componentValue_L = _componentValue_L;
		componentValue_R = _componentValue_R;
		updateComponentResistance();
	}

	/** set L component */
	virtual void setComponentValue_L(double _componentValue_L)
	{
		componentValue_L = _componentValue_L;
		updateComponentResistance();
	}

	/** set R component */
	virtual void setComponentValue_R(double _componentValue_R)
	{
		componentValue_R = _componentValue_R;
		updateComponentResistance();
	}

	/** get L component value */
	virtual double getComponentValue_L() { return componentValue_L; }

	/** get R component value */
	virtual double getComponentValue_R() { return componentValue_R; }

	/** reset the component; clear registers */
	virtual void reset(double _sampleRate) { setSampleRate(_sampleRate); zRegister_L = 0.0; zRegister_C = 0.0; }

	/** set input value into component */
	virtual void setInput(double in){ zRegister_L = in; }

	/** get output value; NOTE: see FX book for details */
	virtual double getOutput()
	{
		double NL = -zRegister_L;
		double out = NL*(1.0 - K) - K*zRegister_C;
		zRegister_C = out;

		return out;
	}

	/** get output1 value; only one resistor output (not used) */
	virtual double getOutput1() { return  getOutput(); }

	/** get output2 value; only one resistor output (not used) */
	virtual double getOutput2() { return  getOutput(); }

	/** get output3 value; only one resistor output (not used) */
	virtual double getOutput3() { return  getOutput(); }

	/** set input1 value; not used for components */
	virtual void setInput1(double _in1) {}

	/** set input2 value; not used for components */
	virtual void setInput2(double _in2) {}

	/** set input3 value; not used for components */
	virtual void setInput3(double _in3) {}

protected:
	double zRegister_L = 0.0; ///< storage register for L
	double zRegister_C = 0.0;///< storage register for C (not used)
	double K = 0.0;

	double componentValue_L = 0.0;///< component value L
	double componentValue_R = 0.0;///< component value R

	double RL = 0.0; ///< RL value
	double RC = 0.0; ///< RC value
	double RR = 0.0; ///< RR value

	double componentResistance = 0.0; ///< equivalent resistance of pair of componen
	double sampleRate = 0.0; ///< sample rate
};

/**
\class WdfParallelRL
\ingroup WDF-Objects
\brief
The WdfParallelRL object implements the reflection coefficient and signal flow through
a WDF simulated parallel RL pair.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfParallelRL : public IComponentAdaptor
{
public:
	WdfParallelRL() {}
	WdfParallelRL(double _componentValue_R, double _componentValue_L)
	{
		componentValue_L = _componentValue_L;
		componentValue_R = _componentValue_R;
	}
	virtual ~WdfParallelRL() {}

	/** set sample rate and update component */
	void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		updateComponentResistance();
	}

	/** get component's value as a resistance */
	virtual double getComponentResistance() { return componentResistance; }

	/** get component's value as a conducatance (or admittance) */
	virtual double getComponentConductance() { return 1.0 / componentResistance; }

	/** change the resistance of component; see FX book for details */
	virtual void updateComponentResistance()
	{
		RR = componentValue_R;
		RL = 2.0*componentValue_L*sampleRate;
		componentResistance = 1.0 / ((1.0 / RR) + (1.0 / RL));
		K = componentResistance / RR;
	}


	/** set both RL components at once */
	virtual void setComponentValue_RL(double _componentValue_R, double _componentValue_L)
	{
		componentValue_L = _componentValue_L;
		componentValue_R = _componentValue_R;
		updateComponentResistance();
	}

	/** set L component */
	virtual void setComponentValue_L(double _componentValue_L)
	{
		componentValue_L = _componentValue_L;
		updateComponentResistance();
	}

	/** set R component */
	virtual void setComponentValue_R(double _componentValue_R)
	{
		componentValue_R = _componentValue_R;
		updateComponentResistance();
	}

	/** get L component value */
	virtual double getComponentValue_L() { return componentValue_L; }

	/** get R component value */
	virtual double getComponentValue_R() { return componentValue_R; }

	/** reset the component; clear registers */
	virtual void reset(double _sampleRate) { setSampleRate(_sampleRate); zRegister_L = 0.0; zRegister_C = 0.0; }

	/** set input value into component */
	virtual void setInput(double in){ zRegister_L = in; }

	/** get output value; NOTE: see FX book for details */
	virtual double getOutput()
	{
		double NL = -zRegister_L;
		double out = NL*(1.0 - K) + K*zRegister_C;
		zRegister_C = out;
		return out;
	}

	/** get output1 value; only one resistor output (not used) */
	virtual double getOutput1() { return  getOutput(); }

	/** get output2 value; only one resistor output (not used) */
	virtual double getOutput2() { return  getOutput(); }

	/** get output3 value; only one resistor output (not used) */
	virtual double getOutput3() { return  getOutput(); }

	/** set input1 value; not used for components */
	virtual void setInput1(double _in1) {}

	/** set input2 value; not used for components */
	virtual void setInput2(double _in2) {}

	/** set input3 value; not used for components */
	virtual void setInput3(double _in3) {}

protected:
	double zRegister_L = 0.0;	///< storage register for L
	double zRegister_C = 0.0;	///< storage register for L
	double K = 0.0;				///< K value

	double componentValue_L = 0.0;	///< component value L
	double componentValue_R = 0.0;	///< component value R

	double RL = 0.0;	///< RL value
	double RC = 0.0;	///< RC value
	double RR = 0.0;	///< RR value

	double componentResistance = 0.0; ///< equivalent resistance of pair of components
	double sampleRate = 0.0; ///< sample rate
};

/**
\class WdfSeriesRC
\ingroup WDF-Objects
\brief
The WdfSeriesRC object implements the reflection coefficient and signal flow through
a WDF simulated series RC pair.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
SEE: p143 "Design of Wave Digital Filters" Psenicka, Ugalde, Romero M.
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfSeriesRC : public IComponentAdaptor
{
public:
	WdfSeriesRC() {}
	WdfSeriesRC(double _componentValue_R, double _componentValue_C)
	{
		componentValue_C = _componentValue_C;
		componentValue_R = _componentValue_R;
	}
	virtual ~WdfSeriesRC() {}

	/** set sample rate and update component */
	void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		updateComponentResistance();
	}

	/** get component's value as a resistance */
	virtual double getComponentResistance() { return componentResistance; }

	/** get component's value as a conducatance (or admittance) */
	virtual double getComponentConductance() { return 1.0 / componentResistance; }

	/** change the resistance of component; see FX book for details */
	virtual void updateComponentResistance()
	{
		RR = componentValue_R;
		RC = 1.0 / (2.0*componentValue_C*sampleRate);
		componentResistance = RR + RC;
		K = RR / componentResistance;
	}

	/** set both RC components at once */
	virtual void setComponentValue_RC(double _componentValue_R, double _componentValue_C)
	{
		componentValue_R = _componentValue_R;
		componentValue_C = _componentValue_C;
		updateComponentResistance();
	}

	/** set R component */
	virtual void setComponentValue_R(double _componentValue_R)
	{
		componentValue_R = _componentValue_R;
		updateComponentResistance();
	}

	/** set C component */
	virtual void setComponentValue_C(double _componentValue_C)
	{
		componentValue_C = _componentValue_C;
		updateComponentResistance();
	}

	/** get R component value */
	virtual double getComponentValue_R() { return componentValue_R; }

	/** get C component value */
	virtual double getComponentValue_C() { return componentValue_C; }

	/** reset the component; clear registers */
	virtual void reset(double _sampleRate) { setSampleRate(_sampleRate); zRegister_L = 0.0; zRegister_C = 0.0; }

	/** set input value into component */
	virtual void setInput(double in){ zRegister_L = in; }

	/** get output value; NOTE: see FX book for details */
	virtual double getOutput()
	{
		double NL = zRegister_L;
		double out = NL*(1.0 - K) + K*zRegister_C;
		zRegister_C = out;
		return out;
	}

	/** get output1 value; only one resistor output (not used) */
	virtual double getOutput1() { return  getOutput(); }

	/** get output2 value; only one resistor output (not used) */
	virtual double getOutput2() { return  getOutput(); }

	/** get output3 value; only one resistor output (not used) */
	virtual double getOutput3() { return  getOutput(); }

	/** set input1 value; not used for components */
	virtual void setInput1(double _in1) {}

	/** set input2 value; not used for components */
	virtual void setInput2(double _in2) {}

	/** set input3 value; not used for components */
	virtual void setInput3(double _in3) {}

protected:
	double zRegister_L = 0.0; ///< storage register for L
	double zRegister_C = 0.0; ///< storage register for C
	double K = 0.0;

	double componentValue_R = 0.0;///< component value R
	double componentValue_C = 0.0;///< component value C

	double RL = 0.0;	///< RL value
	double RC = 0.0;	///< RC value
	double RR = 0.0;	///< RR value

	double componentResistance = 0.0; ///< equivalent resistance of pair of components
	double sampleRate = 0.0; ///< sample rate
};

/**
\class WdfParallelRC
\ingroup WDF-Objects
\brief
The WdfParallelRC object implements the reflection coefficient and signal flow through
a WDF simulated parallal RC pair.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
SEE: p143 "Design of Wave Digital Filters" Psenicka, Ugalde, Romero M.
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfParallelRC : public IComponentAdaptor
{
public:
	WdfParallelRC() {}
	WdfParallelRC(double _componentValue_R, double _componentValue_C)
	{
		componentValue_C = _componentValue_C;
		componentValue_R = _componentValue_R;
	}
	virtual ~WdfParallelRC() {}

	/** set sample rate and update component */
	void setSampleRate(double _sampleRate)
	{
		sampleRate = _sampleRate;
		updateComponentResistance();
	}

	/** get component's value as a resistance */
	virtual double getComponentResistance() { return componentResistance; }

	/** get component's value as a conducatance (or admittance) */
	virtual double getComponentConductance() { return 1.0 / componentResistance; }

	/** change the resistance of component; see FX book for details */
	virtual void updateComponentResistance()
	{
		RR = componentValue_R;
		RC = 1.0 / (2.0*componentValue_C*sampleRate);
		componentResistance = 1.0 / ((1.0 / RR) + (1.0 / RC));
		K = componentResistance / RR;
	}

	/** set both RC components at once */
	virtual void setComponentValue_RC(double _componentValue_R, double _componentValue_C)
	{
		componentValue_R = _componentValue_R;
		componentValue_C = _componentValue_C;
		updateComponentResistance();
	}

	/** set R component */
	virtual void setComponentValue_R(double _componentValue_R)
	{
		componentValue_R = _componentValue_R;
		updateComponentResistance();
	}

	/** set C component */
	virtual void setComponentValue_C(double _componentValue_C)
	{
		componentValue_C = _componentValue_C;
		updateComponentResistance();
	}

	/** get R component value */
	virtual double getComponentValue_R() { return componentValue_R; }

	/** get C component value */
	virtual double getComponentValue_C() { return componentValue_C; }

	/** reset the component; clear registers */
	virtual void reset(double _sampleRate) { setSampleRate(_sampleRate); zRegister_L = 0.0; zRegister_C = 0.0; }

	/** set input value into component; */
	virtual void setInput(double in){ zRegister_L = in; }

	/** get output value; NOTE: output is located in zRegister_C */
	virtual double getOutput()
	{
		double NL = zRegister_L;
		double out = NL*(1.0 - K) - K*zRegister_C;
		zRegister_C = out;
		return out;
	}

	/** get output1 value; only one resistor output (not used) */
	virtual double getOutput1() { return  getOutput(); }

	/** get output2 value; only one resistor output (not used) */
	virtual double getOutput2() { return  getOutput(); }

	/** get output3 value; only one resistor output (not used) */
	virtual double getOutput3() { return  getOutput(); }

	/** set input1 value; not used for components */
	virtual void setInput1(double _in1) {}

	/** set input2 value; not used for components */
	virtual void setInput2(double _in2) {}

	/** set input3 value; not used for components */
	virtual void setInput3(double _in3) {}

protected:
	double zRegister_L = 0.0; ///< storage register for L
	double zRegister_C = 0.0; ///< storage register for C
	double K = 0.0;

	double componentValue_C = 0.0;	///< component value C
	double componentValue_R = 0.0;	///< component value R

	double RL = 0.0; ///< RL value
	double RC = 0.0; ///< RC value
	double RR = 0.0; ///< RR value

	double componentResistance = 0.0; ///< equivalent resistance of pair of components
	double sampleRate = 0.0; ///< sample rate
};


// ------------------------------------------------------------------ //
// --- WDF ADAPTORS ------------------------------------------------- //
// ------------------------------------------------------------------ //

/**
\enum wdfComponent
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the wdfComponent type

- enum class wdfComponent { R, L, C, seriesLC, parallelLC, seriesRL, parallelRL, seriesRC, parallelRC };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class wdfComponent { R, L, C, seriesLC, parallelLC, seriesRL, parallelRL, seriesRC, parallelRC };

/**
\struct WdfComponentInfo
\ingroup WDF-Objects
\brief
Custom structure to hold component information.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct WdfComponentInfo
{
	WdfComponentInfo() { }

	WdfComponentInfo(wdfComponent _componentType, double value1 = 0.0, double value2 = 0.0)
	{
		componentType = _componentType;
		if (componentType == wdfComponent::R)
			R = value1;
		else if (componentType == wdfComponent::L)
			L = value1;
		else if (componentType == wdfComponent::C)
			C = value1;
		else if (componentType == wdfComponent::seriesLC || componentType == wdfComponent::parallelLC)
		{
			L = value1;
			C = value2;
		}
		else if (componentType == wdfComponent::seriesRL || componentType == wdfComponent::parallelRL)
		{
			R = value1;
			L = value2;
		}
		else if (componentType == wdfComponent::seriesRC || componentType == wdfComponent::parallelRC)
		{
			R = value1;
			C = value2;
		}
	}

	double R = 0.0; ///< value of R component
	double L = 0.0;	///< value of L component
	double C = 0.0;	///< value of C component
	wdfComponent componentType = wdfComponent::R; ///< selected component type
};


/**
\class WdfAdaptorBase
\ingroup WDF-Objects
\brief
The WdfAdaptorBase object acts as the base class for all WDF Adaptors; the static members allow
for simplified connection of components. See the FX book for more details.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfAdaptorBase : public IComponentAdaptor
{
public:
	WdfAdaptorBase() {
		if (wdfComponent)
			delete wdfComponent;
	}
	virtual ~WdfAdaptorBase() {}

	/** set the termainal (load) resistance for terminating adaptors */
	void setTerminalResistance(double _terminalResistance) { terminalResistance = _terminalResistance; }

	/** set the termainal (load) resistance as open circuit for terminating adaptors */
	void setOpenTerminalResistance(bool _openTerminalResistance = true)
	{
		// --- flag overrides value
		openTerminalResistance = _openTerminalResistance;
		terminalResistance = 1.0e+34; // avoid /0.0
	}

	/** set the input (source) resistance for an input adaptor */
	void setSourceResistance(double _sourceResistance) { sourceResistance = _sourceResistance; }

	/** set the component or connected adaptor at port 1; functions is generic and allows extending the functionality of the WDF Library */
	void setPort1_CompAdaptor(IComponentAdaptor* _port1CompAdaptor) { port1CompAdaptor = _port1CompAdaptor; }

	/** set the component or connected adaptor at port 2; functions is generic and allows extending the functionality of the WDF Library */
	void setPort2_CompAdaptor(IComponentAdaptor* _port2CompAdaptor) { port2CompAdaptor = _port2CompAdaptor; }

	/** set the component or connected adaptor at port 3; functions is generic and allows extending the functionality of the WDF Library */
	void setPort3_CompAdaptor(IComponentAdaptor* _port3CompAdaptor) { port3CompAdaptor = _port3CompAdaptor; }

	/** reset the connected component */
	virtual void reset(double _sampleRate)
	{
		if (wdfComponent)
			wdfComponent->reset(_sampleRate);
	}

	/** creates a new WDF component and connects it to Port 3 */
	void setComponent(wdfComponent componentType, double value1 = 0.0, double value2 = 0.0)
	{
		if (wdfComponent)
			delete wdfComponent;

		// --- decode and set
		if (componentType == wdfComponent::R)
		{
			wdfComponent = new WdfResistor;
			wdfComponent->setComponentValue(value1);
			port3CompAdaptor = wdfComponent;
		}
		else if (componentType == wdfComponent::L)
		{
			wdfComponent = new WdfInductor;
			wdfComponent->setComponentValue(value1);
			port3CompAdaptor = wdfComponent;
		}
		else if (componentType == wdfComponent::C)
		{
			wdfComponent = new WdfCapacitor;
			wdfComponent->setComponentValue(value1);
			port3CompAdaptor = wdfComponent;
		}
		else if (componentType == wdfComponent::seriesLC)
		{
			wdfComponent = new WdfSeriesLC;
			wdfComponent->setComponentValue_LC(value1, value2);
			port3CompAdaptor = wdfComponent;
		}
		else if (componentType == wdfComponent::parallelLC)
		{
			wdfComponent = new WdfParallelLC;
			wdfComponent->setComponentValue_LC(value1, value2);
			port3CompAdaptor = wdfComponent;
		}
		else if (componentType == wdfComponent::seriesRL)
		{
			wdfComponent = new WdfSeriesRL;
			wdfComponent->setComponentValue_RL(value1, value2);
			port3CompAdaptor = wdfComponent;
		}
		else if (componentType == wdfComponent::parallelRL)
		{
			wdfComponent = new WdfParallelRL;
			wdfComponent->setComponentValue_RL(value1, value2);
			port3CompAdaptor = wdfComponent;
		}
		else if (componentType == wdfComponent::seriesRC)
		{
			wdfComponent = new WdfSeriesRC;
			wdfComponent->setComponentValue_RC(value1, value2);
			port3CompAdaptor = wdfComponent;
		}
		else if (componentType == wdfComponent::parallelRC)
		{
			wdfComponent = new WdfParallelRC;
			wdfComponent->setComponentValue_RC(value1, value2);
			port3CompAdaptor = wdfComponent;
		}
	}

	/** connect two adapters together upstreamAdaptor --> downstreamAdaptor */
	static void connectAdaptors(WdfAdaptorBase* upstreamAdaptor, WdfAdaptorBase* downstreamAdaptor)
	{
		upstreamAdaptor->setPort2_CompAdaptor(downstreamAdaptor);
		downstreamAdaptor->setPort1_CompAdaptor(upstreamAdaptor);
	}

	/** initialize the chain of adaptors from upstreamAdaptor --> downstreamAdaptor */
	virtual void initializeAdaptorChain()
	{
		initialize(sourceResistance);
	}

	/** set value of single-component adaptor */
	virtual void setComponentValue(double _componentValue)
	{
		if (wdfComponent)
			wdfComponent->setComponentValue(_componentValue);
	}

	/** set LC value of mjulti-component adaptor */
	virtual void setComponentValue_LC(double componentValue_L, double componentValue_C)
	{
		if (wdfComponent)
			wdfComponent->setComponentValue_LC(componentValue_L, componentValue_C);
	}

	/** set RL value of mjulti-component adaptor */
	virtual void setComponentValue_RL(double componentValue_R, double componentValue_L)
	{
		if (wdfComponent)
			wdfComponent->setComponentValue_RL(componentValue_R, componentValue_L);
	}

	/** set RC value of mjulti-component adaptor */
	virtual void setComponentValue_RC(double componentValue_R, double componentValue_C)
	{
		if (wdfComponent)
			wdfComponent->setComponentValue_RC(componentValue_R, componentValue_C);
	}

	/** get adaptor connected at port 1: for extended functionality; not used in WDF ladder filter library */
	IComponentAdaptor* getPort1_CompAdaptor() { return port1CompAdaptor; }

	/** get adaptor connected at port 2: for extended functionality; not used in WDF ladder filter library */
	IComponentAdaptor* getPort2_CompAdaptor() { return port2CompAdaptor; }

	/** get adaptor connected at port 3: for extended functionality; not used in WDF ladder filter library */
	IComponentAdaptor* getPort3_CompAdaptor() { return port3CompAdaptor; }

protected:
	// --- can in theory connect any port to a component OR adaptor;
	//     though this library is setup with a convention R3 = component
	IComponentAdaptor* port1CompAdaptor = nullptr;	///< componant or adaptor connected to port 1
	IComponentAdaptor* port2CompAdaptor = nullptr;	///< componant or adaptor connected to port 2
	IComponentAdaptor* port3CompAdaptor = nullptr;	///< componant or adaptor connected to port 3
	IComponentAdaptor* wdfComponent = nullptr;		///< WDF componant connected to port 3 (default operation)

	// --- These hold the input (R1), component (R3) and output (R2) resistances
	double R1 = 0.0; ///< input port resistance
	double R2 = 0.0; ///< output port resistance
	double R3 = 0.0; ///< component resistance

	// --- these are input variables that are stored;
	//     not used in this implementation but may be required for extended versions
	double in1 = 0.0;	///< stored port 1 input;  not used in this implementation but may be required for extended versions
	double in2 = 0.0;	///< stored port 2 input;  not used in this implementation but may be required for extended versions
	double in3 = 0.0;	///< stored port 3 input;  not used in this implementation but may be required for extended versions

	// --- these are output variables that are stored;
	//     currently out2 is the only one used as it is y(n) for this library
	//     out1 and out2 are stored; not used in this implementation but may be required for extended versions
	double out1 = 0.0;	///< stored port 1 output; not used in this implementation but may be required for extended versions
	double out2 = 0.0;	///< stored port 2 output; it is y(n) for this library
	double out3 = 0.0;	///< stored port 3 output; not used in this implementation but may be required for extended versions

	// --- terminal impedance
	double terminalResistance = 600.0; ///< value of terminal (load) resistance
	bool openTerminalResistance = false; ///< flag for open circuit load

	// --- source impedance, OK for this to be set to 0.0 for Rs = 0
	double sourceResistance = 600.0; ///< source impedance; OK for this to be set to 0.0 for Rs = 0
};

/**
\class WdfSeriesAdaptor
\ingroup WDF-Objects
\brief
The WdfSeriesAdaptor object implements the series reflection-free (non-terminated) adaptor

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfSeriesAdaptor : public WdfAdaptorBase
{
public:
	WdfSeriesAdaptor() {}
	virtual ~WdfSeriesAdaptor() {}

	/** get the resistance at port 2; R2 = R1 + component (series)*/
	virtual double getR2()
	{
		double componentResistance = 0.0;
		if (getPort3_CompAdaptor())
			componentResistance = getPort3_CompAdaptor()->getComponentResistance();

		R2 = R1 + componentResistance;
		return R2;
	}

	/** initialize adaptor with input resistance */
	virtual void initialize(double _R1)
	{
		// --- R1 is source resistance for this adaptor
		R1 = _R1;

		double componentResistance = 0.0;
		if (getPort3_CompAdaptor())
			componentResistance = getPort3_CompAdaptor()->getComponentResistance();

		// --- calculate B coeff
		B = R1 / (R1 + componentResistance);

		// --- init downstream adaptor
		if (getPort2_CompAdaptor())
			getPort2_CompAdaptor()->initialize(getR2());

		// --- not used in this implementation but saving for extended use
		R3 = componentResistance;
	}

	/** push audio input sample into incident wave input*/
	virtual void setInput1(double _in1)
	{
		// --- save
		in1 = _in1;

		// --- read component value
		N2 = 0.0;
		if (getPort3_CompAdaptor())
			N2 = getPort3_CompAdaptor()->getOutput();

		// --- form output
		out2 = -(in1 + N2);

		// --- deliver downstream
		if (getPort2_CompAdaptor())
			getPort2_CompAdaptor()->setInput1(out2);
	}

	/** push audio input sample into reflected wave input */
	virtual void setInput2(double _in2)
	{
		// --- save
		in2 = _in2;

		// --- calc N1
		N1 = -(in1 - B*(in1 + N2 + in2) + in2);

		// --- calc out1
		out1 = in1 - B*(N2 + in2);

		// --- deliver upstream
		if (getPort1_CompAdaptor())
			getPort1_CompAdaptor()->setInput2(out1);

		// --- set component state
		if (getPort3_CompAdaptor())
			getPort3_CompAdaptor()->setInput(N1);
	}

	/** set input 3 always connects to component */
	virtual void setInput3(double _in3){ }

	/** get OUT1 = reflected output pin on Port 1 */
	virtual double getOutput1() { return out1; }

	/** get OUT2 = incident (normal) output pin on Port 2 */
	virtual double getOutput2() { return out2; }

	/** get OUT3 always connects to component */
	virtual double getOutput3() { return out3; }

private:
	double N1 = 0.0;	///< node 1 value, internal use only
	double N2 = 0.0;	///< node 2 value, internal use only
	double B = 0.0;		///< B coefficient value
};

/**
\class WdfSeriesTerminatedAdaptor
\ingroup WDF-Objects
\brief
The WdfSeriesTerminatedAdaptor object implements the series terminated (non-reflection-free) adaptor

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
// --- Series terminated adaptor
class WdfSeriesTerminatedAdaptor : public WdfAdaptorBase
{
public:
	WdfSeriesTerminatedAdaptor() {}
	virtual ~WdfSeriesTerminatedAdaptor() {}

	/** get the resistance at port 2; R2 = R1 + component (series)*/
	virtual double getR2()
	{
		double componentResistance = 0.0;
		if (getPort3_CompAdaptor())
			componentResistance = getPort3_CompAdaptor()->getComponentResistance();

		R2 = R1 + componentResistance;
		return R2;
	}

	/** initialize adaptor with input resistance */
	virtual void initialize(double _R1)
	{
		// --- source impedance
		R1 = _R1;

		double componentResistance = 0.0;
		if (getPort3_CompAdaptor())
			componentResistance = getPort3_CompAdaptor()->getComponentResistance();

		B1 = (2.0*R1) / (R1 + componentResistance + terminalResistance);
		B3 = (2.0*terminalResistance) / (R1 + componentResistance + terminalResistance);

		// --- init downstream
		if (getPort2_CompAdaptor())
			getPort2_CompAdaptor()->initialize(getR2());

		// --- not used in this implementation but saving for extended use
		R3 = componentResistance;
	}

	/** push audio input sample into incident wave input*/
	virtual void setInput1(double _in1)
	{
		// --- save
		in1 = _in1;

		N2 = 0.0;
		if (getPort3_CompAdaptor())
			N2 = getPort3_CompAdaptor()->getOutput();

		double N3 = in1 + N2;

		// --- calc out2 y(n)
		out2 = -B3*N3;

		// --- form output1
		out1 = in1 - B1*N3;

		// --- form N1
		N1 = -(out1 + out2 + N3);

		// --- deliver upstream to input2
		if (getPort1_CompAdaptor())
			getPort1_CompAdaptor()->setInput2(out1);

		// --- set component state
		if (getPort3_CompAdaptor())
			getPort3_CompAdaptor()->setInput(N1);
	}

	/** push audio input sample into reflected wave input
	    for terminated adaptor, this is dead end, just store it */
	virtual void setInput2(double _in2) { in2 = _in2;}

	/** set input 3 always connects to component */
	virtual void setInput3(double _in3) { in3 = _in3;}

	/** get OUT1 = reflected output pin on Port 1 */
	virtual double getOutput1() { return out1; }

	/** get OUT2 = incident (normal) output pin on Port 2 */
	virtual double getOutput2() { return out2; }

	/** get OUT3 always connects to component */
	virtual double getOutput3() { return out3; }

private:
	double N1 = 0.0;	///< node 1 value, internal use only
	double N2 = 0.0;	///< node 2 value, internal use only
	double B1 = 0.0;	///< B1 coefficient value
	double B3 = 0.0;	///< B3 coefficient value
};

/**
\class WdfParallelAdaptor
\ingroup WDF-Objects
\brief
The WdfParallelAdaptor object implements the parallel reflection-free (non-terminated) adaptor

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfParallelAdaptor : public WdfAdaptorBase
{
public:
	WdfParallelAdaptor() {}
	virtual ~WdfParallelAdaptor() {}

	/** get the resistance at port 2;  R2 = 1.0/(sum of admittances) */
	virtual double getR2()
	{
		double componentConductance = 0.0;
		if (getPort3_CompAdaptor())
			componentConductance = getPort3_CompAdaptor()->getComponentConductance();

		// --- 1 / (sum of admittances)
		R2 = 1.0 / ((1.0 / R1) + componentConductance);
		return R2;
	}

	/** initialize adaptor with input resistance */
	virtual void initialize(double _R1)
	{
		// --- save R1
		R1 = _R1;

		double G1 = 1.0 / R1;
		double componentConductance = 0.0;
		if (getPort3_CompAdaptor())
			componentConductance = getPort3_CompAdaptor()->getComponentConductance();

		// --- calculate B coeff
		A = G1 / (G1 + componentConductance);

		// --- now, do we init our downstream??
		if (getPort2_CompAdaptor())
			getPort2_CompAdaptor()->initialize(getR2());

		// --- not used in this implementation but saving for extended use
		R3 = 1.0/ componentConductance;
	}

	/** push audio input sample into incident wave input*/
	virtual void setInput1(double _in1)
	{
		// --- save
		in1 = _in1;

		// --- read component
		N2 = 0.0;
		if (getPort3_CompAdaptor())
			N2 = getPort3_CompAdaptor()->getOutput();

		// --- form output
		out2 = N2 - A*(-in1 + N2);

		// --- deliver downstream
		if (getPort2_CompAdaptor())
			getPort2_CompAdaptor()->setInput1(out2);
	}

	/** push audio input sample into reflected wave input*/
	virtual void setInput2(double _in2)
	{
		// --- save
		in2 = _in2;

		// --- calc N1
		N1 = in2 - A*(-in1 + N2);

		// --- calc out1
		out1 = -in1 + N2 + N1;

		// --- deliver upstream
		if (getPort1_CompAdaptor())
			getPort1_CompAdaptor()->setInput2(out1);

		// --- set component state
		if (getPort3_CompAdaptor())
			getPort3_CompAdaptor()->setInput(N1);
	}

	/** set input 3 always connects to component */
	virtual void setInput3(double _in3) { }

	/** get OUT1 = reflected output pin on Port 1 */
	virtual double getOutput1() { return out1; }

	/** get OUT2 = incident (normal) output pin on Port 2 */
	virtual double getOutput2() { return out2; }

	/** get OUT3 always connects to component */
	virtual double getOutput3() { return out3; }

private:
	double N1 = 0.0;	///< node 1 value, internal use only
	double N2 = 0.0;	///< node 2 value, internal use only
	double A = 0.0;		///< A coefficient value
};


/**
\class WdfParallelTerminatedAdaptor
\ingroup WDF-Objects
\brief
The WdfParallelTerminatedAdaptor object implements the parallel  terminated (non-reflection-free) adaptor

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WdfParallelTerminatedAdaptor : public WdfAdaptorBase
{
public:
	WdfParallelTerminatedAdaptor() {}
	virtual ~WdfParallelTerminatedAdaptor() {}

	/** get the resistance at port 2;  R2 = 1.0/(sum of admittances) */
	virtual double getR2()
	{
		double componentConductance = 0.0;
		if (getPort3_CompAdaptor())
			componentConductance = getPort3_CompAdaptor()->getComponentConductance();

		// --- 1 / (sum of admittances)
		R2 = 1.0 / ((1.0 / R1) + componentConductance);
		return R2;
	}

	/** initialize adaptor with input resistance */
	virtual void initialize(double _R1)
	{
		// --- save R1
		R1 = _R1;

		double G1 = 1.0 / R1;
		if (terminalResistance <= 0.0)
			terminalResistance = 1e-15;

		double G2 = 1.0 / terminalResistance;
		double componentConductance = 0.0;
		if (getPort3_CompAdaptor())
			componentConductance = getPort3_CompAdaptor()->getComponentConductance();

		A1 = 2.0*G1 / (G1 + componentConductance + G2);
		A3 = openTerminalResistance ? 0.0 : 2.0*G2 / (G1 + componentConductance + G2);

		// --- init downstream
		if (getPort2_CompAdaptor())
			getPort2_CompAdaptor()->initialize(getR2());

		// --- not used in this implementation but saving for extended use
		R3 = 1.0 / componentConductance;
	}

	/** push audio input sample into incident wave input*/
	virtual void setInput1(double _in1)
	{
		// --- save
		in1 = _in1;

		N2 = 0.0;
		if (getPort3_CompAdaptor())
			N2 = getPort3_CompAdaptor()->getOutput();

		// --- form N1
		N1 = -A1*(-in1 + N2) + N2 - A3*N2;

		// --- form output1
		out1 = -in1 + N2 + N1;

		// --- deliver upstream to input2
		if (getPort1_CompAdaptor())
			getPort1_CompAdaptor()->setInput2(out1);

		// --- calc out2 y(n)
		out2 = N2 + N1;

		// --- set component state
		if (getPort3_CompAdaptor())
			getPort3_CompAdaptor()->setInput(N1);
	}

	/** push audio input sample into reflected wave input; this is a dead end for terminated adaptorsthis is a dead end for terminated adaptors  */
	virtual void setInput2(double _in2){ in2 = _in2;}

	/** set input 3 always connects to component */
	virtual void setInput3(double _in3) { }

	/** get OUT1 = reflected output pin on Port 1 */
	virtual double getOutput1() { return out1; }

	/** get OUT2 = incident (normal) output pin on Port 2 */
	virtual double getOutput2() { return out2; }

	/** get OUT3 always connects to component */
	virtual double getOutput3() { return out3; }

private:
	double N1 = 0.0;	///< node 1 value, internal use only
	double N2 = 0.0;	///< node 2 value, internal use only
	double A1 = 0.0;	///< A1 coefficient value
	double A3 = 0.0;	///< A3 coefficient value
};

// ------------------------------------------------------------------------------ //
// --- WDF Ladder Filter Design  Examples --------------------------------------- //
// ------------------------------------------------------------------------------ //
//
// --- 3rd order Butterworth LPF designed with Elsie www.TonneSoftware.comm
//
/*
	3rd Order Inductor-Leading LPF

	Rs = Rload = 600 ohms

	Series(L1) -> Parallel(C1) -> Series(L2)

	--L1-- | --L2--
		   C1
		   |

	fc = 1kHz
		L1 = 95.49e-3;
		C1 = 0.5305e-6;
		L2 = 95.49e-3;

	fc = 10kHz
		L1 = 9.549e-3;
		C1 = 0.05305e-6;
		L2 = 9.549e-3;
*/

/**
\class WDFButterLPF3
\ingroup WDF-Objects
\brief
The WDFButterLPF3 object implements a 3rd order Butterworth ladder filter.
NOTE: designed with Elsie www.TonneSoftware.comm

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- none - this object is hard-wired.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WDFButterLPF3 : public IAudioSignalProcessor
{
public:
	WDFButterLPF3(void) { createWDF(); }	/* C-TOR */
	~WDFButterLPF3(void) {}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- rest WDF components (flush state registers)
		seriesAdaptor_L1.reset(_sampleRate);
		parallelAdaptor_C1.reset(_sampleRate);
		seriesTerminatedAdaptor_L2.reset(_sampleRate);

		// --- intialize the chain of adapters
		seriesAdaptor_L1.initializeAdaptorChain();
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the WDF ladder filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- push audio sample into series L1
		seriesAdaptor_L1.setInput1(xn);

		// --- output is at terminated L2's output2
		return seriesTerminatedAdaptor_L2.getOutput2();
	}

	/** create the WDF structure for this object - may be called more than once */
	void createWDF()
	{
		// --- actual component values fc = 1kHz
		double L1_value = 95.49e-3;		// 95.5 mH
		double C1_value = 0.5305e-6;	// 0.53 uF
		double L2_value = 95.49e-3;		// 95.5 mH

										// --- set adapter components
		seriesAdaptor_L1.setComponent(wdfComponent::L, L1_value);
		parallelAdaptor_C1.setComponent(wdfComponent::C, C1_value);
		seriesTerminatedAdaptor_L2.setComponent(wdfComponent::L, L2_value);

		// --- connect adapters
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_L1, &parallelAdaptor_C1);
		WdfAdaptorBase::connectAdaptors(&parallelAdaptor_C1, &seriesTerminatedAdaptor_L2);

		// --- set source resistance
		seriesAdaptor_L1.setSourceResistance(600.0); // --- Rs = 600

		// --- set terminal resistance
		seriesTerminatedAdaptor_L2.setTerminalResistance(600.0); // --- Rload = 600
	}

protected:
	// --- three adapters
	WdfSeriesAdaptor seriesAdaptor_L1;			///< adaptor for L1
	WdfParallelAdaptor parallelAdaptor_C1;		///< adaptor for C1
	WdfSeriesTerminatedAdaptor seriesTerminatedAdaptor_L2;	///< adaptor for L2
};

/**
\class WDFTunableButterLPF3
\ingroup WDF-Objects
\brief
The WDFTunableButterLPF3 object implements a tunable 3rd order Butterworth ladder filter.
NOTE: designed with Elsie www.TonneSoftware.comm

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- setUsePostWarping(bool b) to enable/disable warping (see book)
- setFilterFc(double fc_Hz) to set the tunable fc value

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WDFTunableButterLPF3 : public IAudioSignalProcessor
{
public:
	WDFTunableButterLPF3(void) { createWDF(); }	/* C-TOR */
	~WDFTunableButterLPF3(void) {}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		// --- rest WDF components (flush state registers)
		seriesAdaptor_L1.reset(_sampleRate);
		parallelAdaptor_C1.reset(_sampleRate);
		seriesTerminatedAdaptor_L2.reset(_sampleRate);

		// --- intialize the chain of adapters
		seriesAdaptor_L1.initializeAdaptorChain();
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the WDF ladder filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- push audio sample into series L1
		seriesAdaptor_L1.setInput1(xn);

		// --- output is at terminated L2's output2
		return seriesTerminatedAdaptor_L2.getOutput2();
	}

	/** create the filter structure; may be called more than once */
	void createWDF()
	{
		// --- create components, init to noramlized values fc = 1Hz
		seriesAdaptor_L1.setComponent(wdfComponent::L, L1_norm);
		parallelAdaptor_C1.setComponent(wdfComponent::C, C1_norm);
		seriesTerminatedAdaptor_L2.setComponent(wdfComponent::L, L2_norm);

		// --- connect adapters
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_L1, &parallelAdaptor_C1);
		WdfAdaptorBase::connectAdaptors(&parallelAdaptor_C1, &seriesTerminatedAdaptor_L2);

		// --- set source resistance
		seriesAdaptor_L1.setSourceResistance(600.0); // --- Rs = 600

		// --- set terminal resistance
		seriesTerminatedAdaptor_L2.setTerminalResistance(600.0); // --- Rload = 600
	}

	/** parameter setter for warping */
	void setUsePostWarping(bool b) { useFrequencyWarping = b; }

	/** parameter setter for fc */
	void setFilterFc(double fc_Hz)
	{
		if (useFrequencyWarping)
		{
			double arg = (kPi*fc_Hz) / sampleRate;
			fc_Hz = fc_Hz*(tan(arg) / arg);
		}

		seriesAdaptor_L1.setComponentValue(L1_norm / fc_Hz);
		parallelAdaptor_C1.setComponentValue(C1_norm / fc_Hz);
		seriesTerminatedAdaptor_L2.setComponentValue(L2_norm / fc_Hz);
	}

protected:
	// --- three adapters
	WdfSeriesAdaptor seriesAdaptor_L1;		///< adaptor for L1
	WdfParallelAdaptor parallelAdaptor_C1;	///< adaptor for C1
	WdfSeriesTerminatedAdaptor seriesTerminatedAdaptor_L2;	///< adaptor for L2

	double L1_norm = 95.493;		// 95.5 mH
	double C1_norm = 530.516e-6;	// 0.53 uF
	double L2_norm = 95.493;		// 95.5 mH

	bool useFrequencyWarping = false;	///< flag for freq warping
	double sampleRate = 1.0;			///< stored sample rate
};

/**
\class WDFBesselBSF3
\ingroup WDF-Objects
\brief
The WDFBesselBSF3 object implements a 3rd order Bessel BSF
NOTE: designed with Elsie www.TonneSoftware.comm

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- none - object is hardwired

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WDFBesselBSF3 : public IAudioSignalProcessor
{
public:
	WDFBesselBSF3(void) { createWDF(); }	/* C-TOR */
	~WDFBesselBSF3(void) {}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- rest WDF components (flush state registers)
		seriesAdaptor_L1C1.reset(_sampleRate);
		parallelAdaptor_L2C2.reset(_sampleRate);
		seriesTerminatedAdaptor_L3C3.reset(_sampleRate);

		// --- intialize the chain of adapters
		seriesAdaptor_L1C1.initializeAdaptorChain();

		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the WDF ladder filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- push audio sample into series L1
		seriesAdaptor_L1C1.setInput1(xn);

		// --- output is at terminated L2's output2
		return seriesTerminatedAdaptor_L3C3.getOutput2();
	}

	/** create the WDF structure; may be called more than once*/
	void createWDF()
	{
		// --- set component values
		// --- fo = 5kHz
		//     BW = 2kHz or Q = 2.5
		seriesAdaptor_L1C1.setComponent(wdfComponent::parallelLC, 16.8327e-3, 0.060193e-6);	/* L, C */
		parallelAdaptor_L2C2.setComponent(wdfComponent::seriesLC, 49.1978e-3, 0.02059e-6);	/* L, C */
		seriesTerminatedAdaptor_L3C3.setComponent(wdfComponent::parallelLC, 2.57755e-3, 0.393092e-6);	/* L, C */

		// --- connect adapters
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_L1C1, &parallelAdaptor_L2C2);
		WdfAdaptorBase::connectAdaptors(&parallelAdaptor_L2C2, &seriesTerminatedAdaptor_L3C3);

		// --- set source resistance
		seriesAdaptor_L1C1.setSourceResistance(600.0); // Ro = 600

		// --- set terminal resistance
		seriesTerminatedAdaptor_L3C3.setTerminalResistance(600.0);
	}

protected:
	// --- three adapters
	WdfSeriesAdaptor seriesAdaptor_L1C1;		///< adaptor for L1 and C1
	WdfParallelAdaptor parallelAdaptor_L2C2;	///< adaptor for L2 and C2
	WdfSeriesTerminatedAdaptor seriesTerminatedAdaptor_L3C3;	///< adaptor for L3 and C3
};


/**
\class WDFConstKBPF6
\ingroup WDF-Objects
\brief
The WDFConstKBPF6 object implements a 6th order constant K BPF
NOTE: designed with Elsie www.TonneSoftware.comm

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- none - object is hardwired

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WDFConstKBPF6 : public IAudioSignalProcessor
{
public:
	WDFConstKBPF6(void) { createWDF(); }	/* C-TOR */
	~WDFConstKBPF6(void) {}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		// --- rest WDF components (flush state registers)
		seriesAdaptor_L1C1.reset(_sampleRate);
		parallelAdaptor_L2C2.reset(_sampleRate);

		seriesAdaptor_L3C3.reset(_sampleRate);
		parallelAdaptor_L4C4.reset(_sampleRate);

		seriesAdaptor_L5C5.reset(_sampleRate);
		parallelTerminatedAdaptor_L6C6.reset(_sampleRate);

		// --- intialize the chain of adapters
		seriesAdaptor_L1C1.initializeAdaptorChain();
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the WDF ladder filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- push audio sample into series L1
		seriesAdaptor_L1C1.setInput1(xn);

		// --- output is at terminated L6C6 output2
		double output = parallelTerminatedAdaptor_L6C6.getOutput2();

		return output;
	}

	/** create the WDF structure */
	void createWDF()
	{
		// --- fo = 5kHz
		//     BW = 2kHz or Q = 2.5
		seriesAdaptor_L1C1.setComponent(wdfComponent::seriesLC, 47.7465e-3, 0.02122e-6);
		parallelAdaptor_L2C2.setComponent(wdfComponent::parallelLC, 3.81972e-3, 0.265258e-6);

		seriesAdaptor_L3C3.setComponent(wdfComponent::seriesLC, 95.493e-3, 0.01061e-6);
		parallelAdaptor_L4C4.setComponent(wdfComponent::parallelLC, 3.81972e-3, 0.265258e-6);

		seriesAdaptor_L5C5.setComponent(wdfComponent::seriesLC, 95.493e-3, 0.01061e-6);
		parallelTerminatedAdaptor_L6C6.setComponent(wdfComponent::parallelLC, 7.63944e-3, 0.132629e-6);

		// --- connect adapters
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_L1C1, &parallelAdaptor_L2C2);
		WdfAdaptorBase::connectAdaptors(&parallelAdaptor_L2C2, &seriesAdaptor_L3C3);
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_L3C3, &parallelAdaptor_L4C4);
		WdfAdaptorBase::connectAdaptors(&parallelAdaptor_L4C4, &seriesAdaptor_L5C5);
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_L5C5, &parallelTerminatedAdaptor_L6C6);

		// --- set source resistance
		seriesAdaptor_L1C1.setSourceResistance(600.0); // Ro = 600

		// --- set terminal resistance
		parallelTerminatedAdaptor_L6C6.setTerminalResistance(600.0);
	}

protected:
	// --- six adapters
	WdfSeriesAdaptor seriesAdaptor_L1C1;		///< adaptor for L1 and C1
	WdfParallelAdaptor parallelAdaptor_L2C2;	///< adaptor for L2 and C2

	WdfSeriesAdaptor seriesAdaptor_L3C3;		///< adaptor for L3 and C3
	WdfParallelAdaptor parallelAdaptor_L4C4;	///< adaptor for L4 and C4

	WdfSeriesAdaptor seriesAdaptor_L5C5;		///< adaptor for L5 and C5
	WdfParallelTerminatedAdaptor parallelTerminatedAdaptor_L6C6;///< adaptor for L6 and C6
};


/**
\struct WDFParameters
\ingroup WDF-Objects
\brief
Custom parameter structure for the WDF filter examples.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct WDFParameters
{
	WDFParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	WDFParameters& operator=(const WDFParameters& params)
	{
		if (this == &params)
			return *this;

		fc = params.fc;
		Q = params.Q;
		boostCut_dB = params.boostCut_dB;
		frequencyWarping = params.frequencyWarping;
		return *this;
	}

	// --- individual parameters
	double fc = 100.0;				///< filter fc
	double Q = 0.707;				///< filter Q
	double boostCut_dB = 0.0;		///< filter boost or cut in dB
	bool frequencyWarping = true;	///< enable frequency warping
};

/**
\class WDFIdealRLCLPF
\ingroup WDF-Objects
\brief
The WDFIdealRLCLPF object implements an ideal RLC LPF using the WDF library.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use WDFParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WDFIdealRLCLPF : public IAudioSignalProcessor
{
public:
	WDFIdealRLCLPF(void) { createWDF(); }	/* C-TOR */
	~WDFIdealRLCLPF(void) {}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;

		// --- rest WDF components (flush state registers)
		seriesAdaptor_RL.reset(_sampleRate);
		parallelTerminatedAdaptor_C.reset(_sampleRate);

		// --- intialize the chain of adapters
		seriesAdaptor_RL.initializeAdaptorChain();
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the WDF Ideal RLC filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- push audio sample into series L1
		seriesAdaptor_RL.setInput1(xn);

		// --- output is at terminated L2's output2
		//     note compensation scaling by -6dB = 0.5
		//     because of WDF assumption about Rs and Rload
		return 0.5*parallelTerminatedAdaptor_C.getOutput2();
	}

	/** create the WDF structure; may be called more than once */
	void createWDF()
	{
		// --- create components, init to noramlized values fc =
		//	   initial values for fc = 1kHz Q = 0.707
		//     Holding C Constant at 1e-6
		//			   L = 2.533e-2
		//			   R = 2.251131 e2
		seriesAdaptor_RL.setComponent(wdfComponent::seriesRL, 2.251131e2, 2.533e-2);
		parallelTerminatedAdaptor_C.setComponent(wdfComponent::C, 1.0e-6);

		// --- connect adapters
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_RL, &parallelTerminatedAdaptor_C);

		// --- set source resistance
		seriesAdaptor_RL.setSourceResistance(0.0); // --- Rs = 600

		// --- set open ckt termination
		parallelTerminatedAdaptor_C.setOpenTerminalResistance(true);
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return WDFParameters custom data structure
	*/
	WDFParameters getParameters() { return wdfParameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param WDFParameters custom data structure
	*/
	void setParameters(const WDFParameters& _wdfParameters)
	{
		if (_wdfParameters.fc != wdfParameters.fc ||
			_wdfParameters.Q != wdfParameters.Q ||
			_wdfParameters.boostCut_dB != wdfParameters.boostCut_dB ||
			_wdfParameters.frequencyWarping != wdfParameters.frequencyWarping)
		{
			wdfParameters = _wdfParameters;
			double fc_Hz = wdfParameters.fc;

			if (wdfParameters.frequencyWarping)
			{
				double arg = (kPi*fc_Hz) / sampleRate;
				fc_Hz = fc_Hz*(tan(arg) / arg);
			}

			double inductorValue = 1.0 / (1.0e-6 * pow((2.0*kPi*fc_Hz), 2.0));
			double resistorValue = (1.0 / wdfParameters.Q)*(pow(inductorValue / 1.0e-6, 0.5));

			seriesAdaptor_RL.setComponentValue_RL(resistorValue, inductorValue);
			seriesAdaptor_RL.initializeAdaptorChain();
		}
	}

protected:
	WDFParameters wdfParameters;	///< object parameters

	// --- adapters
	WdfSeriesAdaptor				seriesAdaptor_RL;				///< adaptor for series RL
	WdfParallelTerminatedAdaptor	parallelTerminatedAdaptor_C;	///< adaptopr for parallel C

	double sampleRate = 1.0;

};

/**
\class WDFIdealRLCHPF
\ingroup WDF-Objects
\brief
The WDFIdealRLCHPF object implements an ideal RLC HPF using the WDF library.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use WDFParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WDFIdealRLCHPF : public IAudioSignalProcessor
{
public:
	WDFIdealRLCHPF(void) { createWDF(); }	/* C-TOR */
	~WDFIdealRLCHPF(void) {}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		// --- rest WDF components (flush state registers)
		seriesAdaptor_RC.reset(_sampleRate);
		parallelTerminatedAdaptor_L.reset(_sampleRate);

		// --- intialize the chain of adapters
		seriesAdaptor_RC.initializeAdaptorChain();
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the WDF Ideal RLC filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- push audio sample into series L1
		seriesAdaptor_RC.setInput1(xn);

		// --- output is at terminated L2's output2
		//     note compensation scaling by -6dB = 0.5
		//     because of WDF assumption about Rs and Rload
		return 0.5*parallelTerminatedAdaptor_L.getOutput2();
	}

	/** create WDF structure; may be called more than once */
	void createWDF()
	{
		// --- create components, init to noramlized values fc =
		//	   initial values for fc = 1kHz Q = 0.707
		//     Holding C Constant at 1e-6
		//			   L = 2.533e-2
		//			   R = 2.251131 e2
		seriesAdaptor_RC.setComponent(wdfComponent::seriesRC, 2.251131e2, 1.0e-6);
		parallelTerminatedAdaptor_L.setComponent(wdfComponent::L, 2.533e-2);

		// --- connect adapters
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_RC, &parallelTerminatedAdaptor_L);

		// --- set source resistance
		seriesAdaptor_RC.setSourceResistance(0.0); // --- Rs = 600

		// --- set open ckt termination
		parallelTerminatedAdaptor_L.setOpenTerminalResistance(true);
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return WDFParameters custom data structure
	*/
	WDFParameters getParameters() { return wdfParameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param WDFParameters custom data structure
	*/
	void setParameters(const WDFParameters& _wdfParameters)
	{
		if (_wdfParameters.fc != wdfParameters.fc ||
			_wdfParameters.Q != wdfParameters.Q ||
			_wdfParameters.boostCut_dB != wdfParameters.boostCut_dB ||
			_wdfParameters.frequencyWarping != wdfParameters.frequencyWarping)
		{
			wdfParameters = _wdfParameters;
			double fc_Hz = wdfParameters.fc;

			if (wdfParameters.frequencyWarping)
			{
				double arg = (kPi*fc_Hz) / sampleRate;
				fc_Hz = fc_Hz*(tan(arg) / arg);
			}

			double inductorValue = 1.0 / (1.0e-6 * pow((2.0*kPi*fc_Hz), 2.0));
			double resistorValue = (1.0 / wdfParameters.Q)*(pow(inductorValue / 1.0e-6, 0.5));

			seriesAdaptor_RC.setComponentValue_RC(resistorValue, 1.0e-6);
			parallelTerminatedAdaptor_L.setComponentValue(inductorValue);
			seriesAdaptor_RC.initializeAdaptorChain();
		}
	}


protected:
	WDFParameters wdfParameters;	///< object parameters

	// --- three
	WdfSeriesAdaptor				seriesAdaptor_RC;				///< adaptor for RC
	WdfParallelTerminatedAdaptor	parallelTerminatedAdaptor_L;	///< adaptor for L

	double sampleRate = 1.0;	///< sample rate storage
};

/**
\class WDFIdealRLCBPF
\ingroup WDF-Objects
\brief
The WDFIdealRLCBPF object implements an ideal RLC BPF using the WDF library.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use WDFParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WDFIdealRLCBPF : public IAudioSignalProcessor
{
public:
	WDFIdealRLCBPF(void) { createWDF(); }	/* C-TOR */
	~WDFIdealRLCBPF(void) {}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		// --- rest WDF components (flush state registers)
		seriesAdaptor_LC.reset(_sampleRate);
		parallelTerminatedAdaptor_R.reset(_sampleRate);

		// --- intialize the chain of adapters
		seriesAdaptor_LC.initializeAdaptorChain();
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the WDF Ideal RLC filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- push audio sample into series L1
		seriesAdaptor_LC.setInput1(xn);

		// --- output is at terminated L2's output2
		//     note compensation scaling by -6dB = 0.5
		//     because of WDF assumption about Rs and Rload
		return 0.5*parallelTerminatedAdaptor_R.getOutput2();
	}

	/** create the WDF structure*/
	void createWDF()
	{
		// --- create components, init to noramlized values fc =
		//	   initial values for fc = 1kHz Q = 0.707
		//     Holding C Constant at 1e-6
		//			   L = 2.533e-2
		//			   R = 2.251131 e2
		seriesAdaptor_LC.setComponent(wdfComponent::seriesLC, 2.533e-2, 1.0e-6);
		parallelTerminatedAdaptor_R.setComponent(wdfComponent::R, 2.251131e2);

		// --- connect adapters
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_LC, &parallelTerminatedAdaptor_R);

		// --- set source resistance
		seriesAdaptor_LC.setSourceResistance(0.0); // --- Rs = 600

		// --- set open ckt termination
		parallelTerminatedAdaptor_R.setOpenTerminalResistance(true);
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return WDFParameters custom data structure
	*/
	WDFParameters getParameters() { return wdfParameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param WDFParameters custom data structure
	*/
	void setParameters(const WDFParameters& _wdfParameters)
	{
		if (_wdfParameters.fc != wdfParameters.fc ||
			_wdfParameters.Q != wdfParameters.Q ||
			_wdfParameters.boostCut_dB != wdfParameters.boostCut_dB ||
			_wdfParameters.frequencyWarping != wdfParameters.frequencyWarping)
		{
			wdfParameters = _wdfParameters;
			double fc_Hz = wdfParameters.fc;

			if (wdfParameters.frequencyWarping)
			{
				double arg = (kPi*fc_Hz) / sampleRate;
				fc_Hz = fc_Hz*(tan(arg) / arg);
			}

			double inductorValue = 1.0 / (1.0e-6 * pow((2.0*kPi*fc_Hz), 2.0));
			double resistorValue = (1.0 / wdfParameters.Q)*(pow(inductorValue / 1.0e-6, 0.5));

			seriesAdaptor_LC.setComponentValue_LC(inductorValue, 1.0e-6);
			parallelTerminatedAdaptor_R.setComponentValue(resistorValue);
			seriesAdaptor_LC.initializeAdaptorChain();
		}
	}

protected:
	WDFParameters wdfParameters;	///< object parameters

	// --- adapters
	WdfSeriesAdaptor				seriesAdaptor_LC; ///< adaptor for LC
	WdfParallelTerminatedAdaptor	parallelTerminatedAdaptor_R; ///< adaptor for R

	double sampleRate = 1.0;
};


/**
\class WDFIdealRLCBSF
\ingroup WDF-Objects
\brief
The WDFIdealRLCBSF object implements an ideal RLC BSF using the WDF library.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use WDFParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class WDFIdealRLCBSF : public IAudioSignalProcessor
{
public:
	WDFIdealRLCBSF(void) { createWDF(); }	/* C-TOR */
	~WDFIdealRLCBSF(void) {}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		// --- rest WDF components (flush state registers)
		seriesAdaptor_R.reset(_sampleRate);
		parallelTerminatedAdaptor_LC.reset(_sampleRate);

		// --- intialize the chain of adapters
		seriesAdaptor_R.initializeAdaptorChain();
		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the WDF Ideal RLC filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- push audio sample into series L1
		seriesAdaptor_R.setInput1(xn);

		// --- output is at terminated L2's output2
		//     note compensation scaling by -6dB = 0.5
		//     because of WDF assumption about Rs and Rload
		return 0.5*parallelTerminatedAdaptor_LC.getOutput2();
	}

	/** create WDF structure */
	void createWDF()
	{
		// --- create components, init to noramlized values fc =
		//	   initial values for fc = 1kHz Q = 0.707
		//     Holding C Constant at 1e-6
		//			   L = 2.533e-2
		//			   R = 2.251131 e2
		seriesAdaptor_R.setComponent(wdfComponent::R, 2.533e-2);
		parallelTerminatedAdaptor_LC.setComponent(wdfComponent::seriesLC, 2.533e-2, 1.0e-6);

		// --- connect adapters
		WdfAdaptorBase::connectAdaptors(&seriesAdaptor_R, &parallelTerminatedAdaptor_LC);

		// --- set source resistance
		seriesAdaptor_R.setSourceResistance(0.0); // --- Rs = 600

		// --- set open ckt termination
		parallelTerminatedAdaptor_LC.setOpenTerminalResistance(true);
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return WDFParameters custom data structure
	*/
	WDFParameters getParameters() { return wdfParameters; }

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param WDFParameters custom data structure
	*/
	void setParameters(const WDFParameters& _wdfParameters)
	{
		if (_wdfParameters.fc != wdfParameters.fc ||
			_wdfParameters.Q != wdfParameters.Q ||
			_wdfParameters.boostCut_dB != wdfParameters.boostCut_dB ||
			_wdfParameters.frequencyWarping != wdfParameters.frequencyWarping)
		{
			wdfParameters = _wdfParameters;
			double fc_Hz = wdfParameters.fc;

			if (wdfParameters.frequencyWarping)
			{
				double arg = (kPi*fc_Hz) / sampleRate;
				fc_Hz = fc_Hz*(tan(arg) / arg);
			}

			double inductorValue = 1.0 / (1.0e-6 * pow((2.0*kPi*fc_Hz), 2.0));
			double resistorValue = (1.0 / wdfParameters.Q)*(pow(inductorValue / 1.0e-6, 0.5));

			seriesAdaptor_R.setComponentValue(resistorValue);
			parallelTerminatedAdaptor_LC.setComponentValue_LC(inductorValue, 1.0e-6);
			seriesAdaptor_R.initializeAdaptorChain();
		}
	}

protected:
	WDFParameters wdfParameters;	///< object parameters

	// --- adapters
	WdfSeriesAdaptor				seriesAdaptor_R; ///< adaptor for series R
	WdfParallelTerminatedAdaptor	parallelTerminatedAdaptor_LC; ///< adaptor for parallel LC

	double sampleRate = 1.0; ///< sample rate storage
};


// ------------------------------------------------------------------ //
// --- OBJECTS REQUIRING FFTW --------------------------------------- //
// ------------------------------------------------------------------ //

/**
\enum windowType
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set the windowing type for FFT algorithms that use it.

- enum class windowType {kNoWindow, kRectWindow, kHannWindow, kBlackmanHarrisWindow, kHammingWindow };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class windowType {kNoWindow, kRectWindow, kHannWindow, kBlackmanHarrisWindow, kHammingWindow };

/**
@makeWindow
\ingroup FX-Functions

@brief  creates a new std::unique_ptr<double[]> array for a given window lenght and type.

\param windowLength - length of window array (does NOT need to be power of 2)
\param hopSize - hopSize for vococerf applications, may set to 0 for non vocoder use
\param gainCorrectionValue - return variable that contains the window gain correction value
*/
inline std::unique_ptr<double[]> makeWindow(unsigned int windowLength, unsigned int hopSize, windowType window, double& gainCorrectionValue)
{
	std::unique_ptr<double[]> windowBuffer;
	windowBuffer.reset(new double[windowLength]);

	if (!windowBuffer) return nullptr;

	double overlap = hopSize > 0.0 ? 1.0 - (double)hopSize / (double)windowLength : 0.0;
	gainCorrectionValue = 0.0;

	for (int n = 0; n < windowLength; n++)
	{
		if (window == windowType::kRectWindow)
		{
			if (n >= 1 && n <= windowLength - 1)
				windowBuffer[n] = 1.0;
		}
		else if (window == windowType::kHammingWindow)
		{
			windowBuffer[n] = 0.54 - 0.46*cos((n*2.0*kPi) / (windowLength));
		}
		else if (window == windowType::kHannWindow)
		{
			windowBuffer[n] = 0.5 * (1 - cos((n*2.0*kPi) / (windowLength)));
		}
		else if (window == windowType::kBlackmanHarrisWindow)
		{
			windowBuffer[n] = (0.42323 - (0.49755*cos((n*2.0*kPi) / (windowLength))) + 0.07922*cos((2 * n*2.0*kPi) / (windowLength)));
		}
		else if (window == windowType::kNoWindow)
		{
			windowBuffer[n] = 1.0;
		}

		gainCorrectionValue += windowBuffer[n];
	}

	// --- calculate gain correction factor
	if (window != windowType::kNoWindow)
		gainCorrectionValue = (1.0 - overlap) / gainCorrectionValue;
	else
		gainCorrectionValue = 1.0 / gainCorrectionValue;

	return windowBuffer;
}

// --- FFTW ---
#ifdef HAVE_FFTW
#include "fftw3.h"

/**
\class FastFFT
\ingroup FFTW-Objects
\brief
The FastFFT provides a simple wrapper for the FFTW FFT operation - it is ultra-thin and simple to use.

Audio I/O:
- processes mono inputs into FFT outputs.

Control I/F:
- none.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class FastFFT
{
public:
	FastFFT() {}		/* C-TOR */
	~FastFFT() {
		if (windowBuffer) delete[] windowBuffer;
		destroyFFTW();
	}	/* D-TOR */

	/** setup the FFT for a given framelength and window type*/
	void initialize(unsigned int _frameLength, windowType _window);

	/** destroy FFTW objects and plans */
	void destroyFFTW();

	/** do the FFT and return real and imaginary arrays */
	fftw_complex* doFFT(double* inputReal, double* inputImag = nullptr);

	/** do the IFFT and return real and imaginary arrays */
	fftw_complex* doInverseFFT(double* inputReal, double* inputImag);

	/** get the current FFT length */
	unsigned int getFrameLength() { return frameLength; }

protected:
	// --- setup FFTW
	fftw_complex*	fft_input = nullptr;		///< array for FFT input
	fftw_complex*	fft_result = nullptr;		///< array for FFT output
	fftw_complex*	ifft_input = nullptr;		///< array for IFFT input
	fftw_complex*	ifft_result = nullptr;		///< array for IFFT output
	fftw_plan       plan_forward = nullptr;		///< FFTW plan for FFT
	fftw_plan		plan_backward = nullptr;	///< FFTW plan for IFFT

	double* windowBuffer = nullptr;				///< buffer for window (naked)
	double windowGainCorrection = 1.0;			///< window gain correction
	windowType window = windowType::kHannWindow; ///< window type
	unsigned int frameLength = 0;				///< current FFT length
};


/**
\class PhaseVocoder
\ingroup FFTW-Objects
\brief
The PhaseVocoder provides a basic phase vocoder that is initialized to N = 4096 and
75% overlap; the de-facto standard for PSM algorithms. The analysis and sythesis
hop sizes are identical.

Audio I/O:
- processes mono input into mono output.

Control I/F:
- none.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class PhaseVocoder
{
public:
	PhaseVocoder() {}		/* C-TOR */
	~PhaseVocoder() {
		if (inputBuffer) delete[] inputBuffer;
		if (outputBuffer) delete[] outputBuffer;
		if (windowBuffer) delete[] windowBuffer;
		destroyFFTW();
	}	/* D-TOR */

	/** setup the FFT for a given framelength and window type*/
	void initialize(unsigned int _frameLength, unsigned int _hopSize, windowType _window);

	/** destroy FFTW objects and plans */
	void destroyFFTW();

	/** process audio sample through vocode; check fftReady flag to access FFT output */
	double processAudioSample(double input, bool& fftReady);

	/** add zero-padding without advancing output read location, for fast convolution */
	bool addZeroPad(unsigned int count);

	/** increment the FFT counter and do the FFT if it is ready */
	bool advanceAndCheckFFT();

	/** get FFT data for manipulation (yes, naked pointer so you can manipulate) */
	fftw_complex* getFFTData() { return fft_result; }

	/** get IFFT data for manipulation (yes, naked pointer so you can manipulate) */
	fftw_complex* getIFFTData() { return ifft_result; }

	/** do the inverse FFT (optional; will be called automatically if not used) */
	void doInverseFFT();

	/** do the overlap-add operation */
	void doOverlapAdd(double* outputData = nullptr, int length = 0);

	/** get current FFT length */
	unsigned int getFrameLength() { return frameLength; }

	/** get current hop size ha = hs */
	unsigned int getHopSize() { return hopSize; }

	/** get current overlap as a raw value (75% = 0.75) */
	double getOverlap() { return overlap; }

	/** set the vocoder for overlap add only without hop-size */
	// --- for fast convolution and other overlap-add algorithms
	//     that are not hop-size dependent
	void setOverlapAddOnly(bool b){ bool overlapAddOnly = b; }

protected:
	// --- setup FFTW
	fftw_complex*	fft_input = nullptr;		///< array for FFT input
	fftw_complex*	fft_result = nullptr;		///< array for FFT output
	fftw_complex*	ifft_result = nullptr;		///< array for IFFT output
	fftw_plan       plan_forward = nullptr;		///< FFTW plan for FFT
	fftw_plan		plan_backward = nullptr;	///< FFTW plan for IFFT

	// --- linear buffer for window
	double*			windowBuffer = nullptr;		///< array for window

	// --- circular buffers for input and output
	double*			inputBuffer = nullptr;		///< input timeline (x)
	double*			outputBuffer = nullptr;		///< output timeline (y)

	// --- index and wrap masks for input and output buffers
	unsigned int inputWriteIndex = 0;			///< circular buffer index: input write
	unsigned int outputWriteIndex = 0;			///< circular buffer index: output write
	unsigned int inputReadIndex = 0;			///< circular buffer index: input read
	unsigned int outputReadIndex = 0;			///< circular buffer index: output read
	unsigned int wrapMask = 0;					///< input wrap mask
	unsigned int wrapMaskOut = 0;				///< output wrap mask

	// --- amplitude correction factor, aking into account both hopsize (overlap)
	//     and the window power itself
	double windowHopCorrection = 1.0;			///< window correction including hop/overlap

	// --- these allow a more robust combination of user interaction
	bool needInverseFFT = false;				///< internal flag to signal IFFT required
	bool needOverlapAdd = false;				///< internal flag to signal overlap/add required

	// --- our window type; you can add more windows if you like
	windowType window = windowType::kHannWindow;///< window type

	// --- counters
	unsigned int frameLength = 0;				///< current FFT length
	unsigned int fftCounter = 0;				///< FFT sample counter

	// --- hop-size and overlap (mathematically related)
	unsigned int hopSize = 0;					///< hop: ha = hs
	double overlap = 1.0;						///< overlap as raw value (75% = 0.75)

	// --- flag for overlap-add algorithms that do not involve hop-size, other
	//     than setting the overlap
	bool overlapAddOnly = false;				///< flag for overlap-add-only algorithms

};

/**
\class FastConvolver
\ingroup FFTW-Objects
\brief
The FastConvolver provides a fast convolver - the user supplies the filter IR and the object
snapshots the FFT of that filter IR. Input audio is fast-convovled with the filter FFT using
complex multiplication and zero-padding.

Audio I/O:
- processes mono input into mono output.

Control I/F:
- none.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class FastConvolver
{
public:
	FastConvolver() {
		vocoder.setOverlapAddOnly(true);
	}		/* C-TOR */
	~FastConvolver() {
		if (filterIR)
			delete[] filterIR;

		if (filterFFT)
			fftw_free(filterFFT);
	}	/* D-TOR */

	/** setup the FFT for a given IR length */
	/**
	\param _filterImpulseLength the filter IR length, which is 1/2 FFT length due to need for zero-padding (see FX book)
	*/
	void initialize(unsigned int _filterImpulseLength)
	{
		if (filterImpulseLength == _filterImpulseLength)
			return;

		// --- setup a specialized vocoder with 50% hop size
		filterImpulseLength = _filterImpulseLength;
		vocoder.initialize(filterImpulseLength * 2, filterImpulseLength, windowType::kNoWindow);

		// --- initialize the FFT object for capturing the filter FFT
		filterFastFFT.initialize(filterImpulseLength * 2, windowType::kNoWindow);

		// --- array to hold the filter IR; this could be localized to the particular function that uses it
		if (filterIR)
			delete [] filterIR;
		filterIR = new double[filterImpulseLength * 2];
		memset(&filterIR[0], 0, filterImpulseLength * 2 * sizeof(double));

		// --- allocate the filter FFT arrays
		if(filterFFT)
			fftw_free(filterFFT);

		 filterFFT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * filterImpulseLength * 2);

		 // --- reset
		 inputCount = 0;
	}

	/** setup the filter IRirBuffer MUST be exactly filterImpulseLength in size, or this will crash! */
	void setFilterIR(double* irBuffer)
	{
		if (!irBuffer) return;

		memset(&filterIR[0], 0, filterImpulseLength * 2 * sizeof(double));

		// --- copy over first half; filterIR len = filterImpulseLength * 2
		int m = 0;
		for (unsigned int i = 0; i < filterImpulseLength; i++)
		{
			filterIR[i] = irBuffer[i];
		}

		// --- take FFT of the h(n)
		fftw_complex* fftOfFilter = filterFastFFT.doFFT(&filterIR[0]);

		// --- copy the FFT into our local buffer for storage; also
		//     we never want to hold a pointer to a FFT output
		//     for more than one local function's worth
		//     could replace with memcpy( )
		for (int i = 0; i < 2; i++)
		{
			for (unsigned int j = 0; j < filterImpulseLength * 2; j++)
			{
				filterFFT[j][i] = fftOfFilter[j][i];
			}
		}
	}

	/** process an input sample through convolver */
	double processAudioSample(double input)
	{
		bool fftReady = false;
		double output = 0.0;

		if (inputCount == filterImpulseLength)
		{
			fftReady = vocoder.addZeroPad(filterImpulseLength);

			if (fftReady) // should happen on time
			{
				// --- multiply our filter IR with the vocoder FFT
				fftw_complex* signalFFT = vocoder.getFFTData();
				if (signalFFT)
				{
					unsigned int fff = vocoder.getFrameLength();

					// --- complex multiply with FFT of IR
					for (unsigned int i = 0; i < filterImpulseLength * 2; i++)
					{
						// --- get real/imag parts of each FFT
						ComplexNumber signal(signalFFT[i][0], signalFFT[i][1]);
						ComplexNumber filter(filterFFT[i][0], filterFFT[i][1]);

						// --- use complex multiply function; this convolves in the time domain
						ComplexNumber product = complexMultiply(signal, filter);

						// --- overwrite the FFT bins
						signalFFT[i][0] = product.real;
						signalFFT[i][1] = product.imag;
					}
				}
			}

			// --- reset counter
			inputCount = 0;
		}

		// --- process next sample
		output = vocoder.processAudioSample(input, fftReady);
		inputCount++;

		return output;
	}

	/** get current frame length */
	unsigned int getFrameLength() { return vocoder.getFrameLength(); }

	/** get current IR length*/
	unsigned int getFilterIRLength() { return filterImpulseLength; }

protected:
	PhaseVocoder vocoder;				///< vocoder object
	FastFFT filterFastFFT;				///< FastFFT object
	fftw_complex* filterFFT = nullptr;	///< filterFFT output arrays
	double* filterIR = nullptr;			///< filter IR
	unsigned int inputCount = 0;		///< input sample counter
	unsigned int filterImpulseLength = 0;///< IR length
};

// --- PSM Vocoder
const unsigned int PSM_FFT_LEN = 4096;

/**
\struct BinData
\ingroup FFTW-Objects
\brief
Custom structure that holds information about each FFT bin. This includes all information
needed to perform pitch shifting and time stretching with phase locking (optional)
and peak tracking (optional).

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct BinData
{
	BinData() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	BinData& operator=(const BinData& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		isPeak = params.isPeak;
		magnitude = params.magnitude;
		phi = params.phi;

		psi = params.psi;
		localPeakBin = params.localPeakBin;
		previousPeakBin = params.previousPeakBin;
		updatedPhase = params.updatedPhase;

		return *this;
	}

	/** reset all variables to 0.0 */
	void reset()
	{
		isPeak = false;
		magnitude = 0.0;
		phi = 0.0;

		psi = 0.0;
		localPeakBin = 0;
		previousPeakBin = -1; // -1 is flag
		updatedPhase = 0.0;
	}

	bool isPeak = false;	///< flag for peak bins
	double magnitude = 0.0; ///< bin magnitude angle
	double phi = 0.0;		///< bin phase angle
	double psi = 0.0;		///< bin phase correction
	unsigned int localPeakBin = 0; ///< index of peak-boss
	int previousPeakBin = -1; ///< index of peak bin in previous FFT
	double updatedPhase = 0.0; ///< phase update value
};

/**
\struct PSMVocoderParameters
\ingroup FFTW-Objects
\brief
Custom parameter structure for the Biquad object. Default version defines the biquad structure used in the calculation.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct PSMVocoderParameters
{
	PSMVocoderParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	PSMVocoderParameters& operator=(const PSMVocoderParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		pitchShiftSemitones = params.pitchShiftSemitones;
		enablePeakPhaseLocking = params.enablePeakPhaseLocking;
		enablePeakTracking = params.enablePeakTracking;

		return *this;
	}

	// --- params
	double pitchShiftSemitones = 0.0;	///< pitch shift in half-steps
	bool enablePeakPhaseLocking = false;///< flag to enable phase lock
	bool enablePeakTracking = false;	///< flag to enable peak tracking
};

/**
\class PSMVocoder
\ingroup FFTW-Objects
\brief
The PSMVocoder object implements a phase vocoder pitch shifter. Phase locking and peak tracking
are optional.

Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use PSMVocoderParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class PSMVocoder : public IAudioSignalProcessor
{
public:
	PSMVocoder() {
		vocoder.initialize(PSM_FFT_LEN, PSM_FFT_LEN/4, windowType::kHannWindow);  // 75% overlap
	}		/* C-TOR */
	~PSMVocoder() {
		if (windowBuff) delete[] windowBuff;
		if (outputBuff) delete[] outputBuff;

	}	/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		memset(&phi[0], 0, sizeof(double)*PSM_FFT_LEN);
		memset(&psi[0], 0, sizeof(double)* PSM_FFT_LEN);
		if(outputBuff)
			memset(outputBuff, 0, sizeof(double)*outputBufferLength);

		for (int i = 0; i < PSM_FFT_LEN; i++)
		{
			binData[i].reset();
			binDataPrevious[i].reset();

			peakBins[i] = -1;
			peakBinsPrevious[i] = -1;
		}

		return true;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** set the pitch shift in semitones (note that this can be fractional too)*/
	void setPitchShift(double semitones)
	{
		// --- this is costly so only update when things changed
		double newAlpha = pow(2.0, semitones / 12.0);
		double newOutputBufferLength = round((1.0/newAlpha)*(double)PSM_FFT_LEN);

		// --- check for change
		if (newOutputBufferLength == outputBufferLength)
			return;

		// --- new stuff
		alphaStretchRatio = newAlpha;
		ha = hs / alphaStretchRatio;

		// --- set output resample buffer
		outputBufferLength = newOutputBufferLength;

		// --- create Hann window
		if (windowBuff) delete[] windowBuff;
		windowBuff = new double[outputBufferLength];
		windowCorrection = 0.0;
		for (unsigned int i = 0; i < outputBufferLength; i++)
		{
			windowBuff[i] = 0.5 * (1.0 - cos((i*2.0*kPi) / (outputBufferLength)));
			windowCorrection += windowBuff[i];
		}
		windowCorrection = 1.0 / windowCorrection;

		// --- create output buffer
		if (outputBuff) delete[] outputBuff;
		outputBuff = new double[outputBufferLength];
		memset(outputBuff, 0, sizeof(double)*outputBufferLength);
	}

	/** find bin index of nearest peak bin in previous FFT frame */
	int findPreviousNearestPeak(int peakIndex)
	{
		if (peakBinsPrevious[0] == -1) // first run, there is no peak
			return -1;

		int delta = -1;
		int previousPeak = -1;
		for (int i = 0; i < PSM_FFT_LEN; i++)
		{
			if (peakBinsPrevious[i] < 0)
				break;

			int dist = abs(peakIndex - peakBinsPrevious[i]);
			if (dist > PSM_FFT_LEN/4)
				break;

			if (i == 0)
			{
				previousPeak = i;
				delta = dist;
			}
			else if (dist < delta)
			{
				previousPeak = i;
				delta = dist;
			}
		}

		return previousPeak;
	}

	/** identify peak bins and tag their respective regions of influence */
	void findPeaksAndRegionsOfInfluence()
	{
		// --- FIND PEAKS --- //
		//
		// --- find local maxima in 4-sample window
		double localWindow[4] = { 0.0 };
		int m = 0;
		for (int i = 0; i < PSM_FFT_LEN; i++)
		{
			if (i == 0)
			{
				localWindow[0] = 0.0;
				localWindow[1] = 0.0;
				localWindow[2] = binData[i + 1].magnitude;
				localWindow[3] = binData[i + 2].magnitude;
			}
			else  if (i == 1)
			{
				localWindow[0] = 0.0;
				localWindow[1] = binData[i - 1].magnitude;
				localWindow[2] = binData[i + 1].magnitude;
				localWindow[3] = binData[i + 2].magnitude;
			}
			else  if (i == PSM_FFT_LEN - 1)
			{
				localWindow[0] = binData[i - 2].magnitude;
				localWindow[1] = binData[i - 1].magnitude;
				localWindow[2] = 0.0;
				localWindow[3] = 0.0;
			}
			else  if (i == PSM_FFT_LEN - 2)
			{
				localWindow[0] = binData[i - 2].magnitude;
				localWindow[1] = binData[i - 1].magnitude;
				localWindow[2] = binData[i + 1].magnitude;
				localWindow[3] = 0.0;
			}
			else
			{
				localWindow[0] = binData[i - 2].magnitude;
				localWindow[1] = binData[i - 1].magnitude;
				localWindow[2] = binData[i + 1].magnitude;
				localWindow[3] = binData[i + 2].magnitude;
			}

			// --- found peak bin!
			if (binData[i].magnitude > 0.00001 &&
				binData[i].magnitude > localWindow[0]
				&& binData[i].magnitude > localWindow[1]
				&& binData[i].magnitude > localWindow[2]
				&& binData[i].magnitude > localWindow[3])
			{
				binData[i].isPeak = true;
				peakBins[m++] = i;

				// --- for peak bins, assume that it is part of a previous, moving peak
				if (parameters.enablePeakTracking)
					binData[i].previousPeakBin = findPreviousNearestPeak(i);
				else
					binData[i].previousPeakBin = -1;
			}
		}

		// --- assign peak bosses
		if (m > 0)
		{
			int n = 0;
			int bossPeakBin = peakBins[n];
			double nextPeak = peakBins[++n];
			int midBoundary = (nextPeak - (double)bossPeakBin) / 2.0 + bossPeakBin;

			if (nextPeak >= 0)
			{
				for (int i = 0; i < PSM_FFT_LEN; i++)
				{
					if (i <= bossPeakBin)
					{
						binData[i].localPeakBin = bossPeakBin;
					}
					else if (i < midBoundary)
					{
						binData[i].localPeakBin = bossPeakBin;
					}
					else // > boundary, calc next set
					{
						bossPeakBin = nextPeak;
						nextPeak = peakBins[++n];
						if (nextPeak > bossPeakBin)
							midBoundary = (nextPeak - (double)bossPeakBin) / 2.0 + bossPeakBin;
						else // nextPeak == -1
							midBoundary = PSM_FFT_LEN;

						binData[i].localPeakBin = bossPeakBin;
					}
				}
			}
		}
	}

	/** process input sample through PSM vocoder */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double input)
	{
			bool fftReady = false;
			double output = 0.0;

			// --- normal processing
			output = vocoder.processAudioSample(input, fftReady);

			// --- if FFT is here, GO!
			if (fftReady)
			{
			// --- get the FFT data
			fftw_complex* fftData = vocoder.getFFTData();

			if (parameters.enablePeakPhaseLocking)
			{
				// --- get the magnitudes for searching
				for (int i = 0; i < PSM_FFT_LEN; i++)
				{
					binData[i].reset();
					peakBins[i] = -1;

					// --- store mag and phase
					binData[i].magnitude = getMagnitude(fftData[i][0], fftData[i][1]);
					binData[i].phi = getPhase(fftData[i][0], fftData[i][1]);
				}

				findPeaksAndRegionsOfInfluence();

				// --- each bin data should now know its local boss-peak
				//
				// --- now propagate phases accordingly
				//
				//     FIRST: set PSI angles of bosses
				for (int i = 0; i < PSM_FFT_LEN; i++)
				{
					double mag_k = binData[i].magnitude;
					double phi_k = binData[i].phi;

					// --- horizontal phase propagation
					//
					// --- omega_k = bin frequency(k)
					double omega_k = kTwoPi*i / PSM_FFT_LEN;

					// --- phase deviation is actual - expected phase
					//     = phi_k -(phi(last frame) + wk*ha
					double phaseDev = phi_k - phi[i] - omega_k*ha;

					// --- unwrapped phase increment
					double deltaPhi = omega_k*ha + principalArg(phaseDev);

					// --- save for next frame
					phi[i] = phi_k;

					// --- if peak, assume it could have hopped from a different bin
					if (binData[i].isPeak)
					{
						// --- calculate new phase based on stretch factor; save phase for next time
						if(binData[i].previousPeakBin < 0)
							psi[i] = principalArg(psi[i] + deltaPhi * alphaStretchRatio);
						else
							psi[i] = principalArg(psi[binDataPrevious[i].previousPeakBin] + deltaPhi * alphaStretchRatio);
					}

					// --- save peak PSI (new angle)
					binData[i].psi = psi[i];

					// --- for IFFT
					binData[i].updatedPhase = binData[i].psi;
				}

				// --- now set non-peaks
				for (int i = 0; i < PSM_FFT_LEN; i++)
				{
					if (!binData[i].isPeak)
					{
						int myPeakBin = binData[i].localPeakBin;

						double PSI_kp = binData[myPeakBin].psi;
						double phi_kp = binData[myPeakBin].phi;

						// --- save for next frame
						// phi[i] = binData[myPeakBin].phi;

						// --- calculate new phase, locked to boss peak
						psi[i] = principalArg(PSI_kp - phi_kp - binData[i].phi);
						binData[i].updatedPhase = psi[i];// principalArg(PSI_kp - phi_kp - binData[i].phi);
					}
				}

				for (int i = 0; i < PSM_FFT_LEN; i++)
				{
					double mag_k = binData[i].magnitude;

					// --- convert back
					fftData[i][0] = mag_k*cos(binData[i].updatedPhase);
					fftData[i][1] = mag_k*sin(binData[i].updatedPhase);

					// --- save for next frame
					binDataPrevious[i] = binData[i];
					peakBinsPrevious[i] = peakBins[i];

				}
			}// end if peak locking

			else // ---> old school
			{
				for (int i = 0; i < PSM_FFT_LEN; i++)
				{
					double mag_k = getMagnitude(fftData[i][0], fftData[i][1]);
					double phi_k = getPhase(fftData[i][0], fftData[i][1]);

					// --- horizontal phase propagation
					//
					// --- omega_k = bin frequency(k)
					double omega_k = kTwoPi*i / PSM_FFT_LEN;

					// --- phase deviation is actual - expected phase
					//     = phi_k -(phi(last frame) + wk*ha
					double phaseDev = phi_k - phi[i] - omega_k*ha;

					// --- unwrapped phase increment
					double deltaPhi = omega_k*ha + principalArg(phaseDev);

					// --- save for next frame
					phi[i] = phi_k;

					// --- calculate new phase based on stretch factor; save phase for next time
					psi[i] = principalArg(psi[i] + deltaPhi * alphaStretchRatio);

					// --- convert back
					fftData[i][0] = mag_k*cos(psi[i]);
					fftData[i][1] = mag_k*sin(psi[i]);
				}
			}


			// --- manually so the IFFT (OPTIONAL)
			vocoder.doInverseFFT();

			// --- can get the iFFT buffers
			fftw_complex* inv_fftData = vocoder.getIFFTData();

			// --- make copy (can speed this up)
			double ifft[PSM_FFT_LEN] = { 0.0 };
			for (int i = 0; i < PSM_FFT_LEN; i++)
				ifft[i] = inv_fftData[i][0];

			// --- resample the audio as if it were stretched
			resample(&ifft[0], outputBuff, PSM_FFT_LEN, outputBufferLength, interpolation::kLinear, windowCorrection, windowBuff);

			// --- overlap-add the interpolated buffer to complete the operation
			vocoder.doOverlapAdd(&outputBuff[0], outputBufferLength);
			}

			return output;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return PSMVocoderParameters custom data structure
	*/
	PSMVocoderParameters getParameters()
	{
		return parameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param PSMVocoderParameters custom data structure
	*/
	void setParameters(const PSMVocoderParameters& params)
	{
		if (params.pitchShiftSemitones != parameters.pitchShiftSemitones)
		{
			setPitchShift(params.pitchShiftSemitones);
		}

		// --- save
		parameters = params;
	}

protected:
	PSMVocoderParameters parameters;	///< object parameters
	PhaseVocoder vocoder;				///< vocoder to perform PSM
	double alphaStretchRatio = 1.0;		///< alpha stretch ratio = hs/ha

	// --- FFT is 4096 with 75% overlap
	const double hs = PSM_FFT_LEN / 4;	///< hs = N/4 --- 75% overlap
	double ha = PSM_FFT_LEN / 4;		///< ha = N/4 --- 75% overlap
	double phi[PSM_FFT_LEN] = { 0.0 };	///< array of phase values for classic algorithm
	double psi[PSM_FFT_LEN] = { 0.0 };	///< array of phase correction values for classic algorithm

	// --- for peak-locking
	BinData binData[PSM_FFT_LEN];			///< array of BinData structures for current FFT frame
	BinData binDataPrevious[PSM_FFT_LEN];	///< array of BinData structures for previous FFT frame

	int peakBins[PSM_FFT_LEN] = { -1 };		///< array of current peak bin index values (-1 = not peak)
	int peakBinsPrevious[PSM_FFT_LEN] = { -1 }; ///< array of previous peak bin index values (-1 = not peak)

	double* windowBuff = nullptr;			///< buffer for window
	double* outputBuff = nullptr;			///< buffer for resampled output
	double windowCorrection = 0.0;			///< window correction value
	unsigned int outputBufferLength = 0;	///< lenght of resampled output array
};

// --- sample rate conversion
//
// --- supported conversion ratios - you can EASILY add more to this
/**
\enum rateConversionRatio
\ingroup Constants-Enums
\brief
Use this strongly typed enum to easily set up or down sampling ratios.

- enum class rateConversionRatio { k2x, k4x };

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
enum class rateConversionRatio { k2x, k4x };
const unsigned int maxSamplingRatio = 4;

/**
@countForRatio
\ingroup FX-Functions

@brief returns the up or downsample ratio as a numeric value

\param ratio - enum class ratio value
\return the up or downsample ratio as a numeric value
*/
inline unsigned int countForRatio(rateConversionRatio ratio)
{
	if (ratio == rateConversionRatio::k2x)
		return 2;
	else if (ratio == rateConversionRatio::k4x || ratio == rateConversionRatio::k4x)
		return 4;

	return 0;
}

// --- get table pointer for built-in anti-aliasing LPFs
/**
@getFilterIRTable
\ingroup FX-Functions

@brief returns the up or downsample ratio as a numeric value

\param FIRLength - lenght of FIR
\param ratio - the conversinon ratio
\param sampleRate - the sample rate
\return a pointer to the appropriate FIR coefficient table in filters.h or nullptr if not found
*/
inline double* getFilterIRTable(unsigned int FIRLength, rateConversionRatio ratio, unsigned int sampleRate)
{
	// --- we only have built in filters for 44.1 and 48 kHz
	if (sampleRate != 44100 && sampleRate != 48000) return nullptr;

	// --- choose 2xtable
	if (ratio == rateConversionRatio::k2x)
	{
		if (sampleRate == 44100)
		{
			if (FIRLength == 128)
				return &LPF128_882[0];
			else if (FIRLength == 256)
				return &LPF256_882[0];
			else if (FIRLength == 512)
				return &LPF512_882[0];
			else if (FIRLength == 1024)
				return &LPF1024_882[0];
		}
		if (sampleRate == 48000)
		{
			if (FIRLength == 128)
				return &LPF128_96[0];
			else if (FIRLength == 256)
				return &LPF256_96[0];
			else if (FIRLength == 512)
				return &LPF512_96[0];
			else if (FIRLength == 1024)
				return &LPF1024_96[0];
		}
	}

	// --- choose 4xtable
	if (ratio == rateConversionRatio::k4x)
	{
		if (sampleRate == 44100)
		{
			if (FIRLength == 128)
				return &LPF128_1764[0];
			else if (FIRLength == 256)
				return &LPF256_1764[0];
			else if (FIRLength == 512)
				return &LPF512_1764[0];
			else if (FIRLength == 1024)
				return &LPF1024_1764[0];
		}
		if (sampleRate == 48000)
		{
			if (FIRLength == 128)
				return &LPF128_192[0];
			else if (FIRLength == 256)
				return &LPF256_192[0];
			else if (FIRLength == 512)
				return &LPF512_192[0];
			else if (FIRLength == 1024)
				return &LPF1024_192[0];
		}
	}
	return nullptr;
}

// --- get table pointer for built-in anti-aliasing LPFs
/**
@decomposeFilter
\ingroup FX-Functions

@brief performs a polyphase decomposition on a big FIR into a set of sub-band FIRs

\param filterIR - pointer to filter IR array
\param FIRLength - lenght of IR array
\param ratio - up or down sampling ratio
\return a pointer an arry of buffer pointers to the decomposed mini-filters
*/
inline double** decomposeFilter(double* filterIR, unsigned int FIRLength, unsigned int ratio)
{
	unsigned int subBandLength = FIRLength / ratio;
	double ** polyFilterSet = new double*[ratio];
	for (unsigned int i = 0; i < ratio; i++)
	{
		double* polyFilter = new double[subBandLength];
		polyFilterSet[i] = polyFilter;
	}

	int m = 0;
	for (int i = 0; i < subBandLength; i++)
	{
		for (int j = ratio - 1; j >= 0; j--)
		{
			double* polyFilter = polyFilterSet[j];
			polyFilter[i] = filterIR[m++];
		}
	}

	return polyFilterSet;
}

/**
\struct InterpolatorOutput
\ingroup FFTW-Objects
\brief
Custom output structure for interpolator; it holds an arry of interpolated output samples.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct InterpolatorOutput
{
	InterpolatorOutput() {}
	double audioData[maxSamplingRatio] = { 0.0 };	///< array of interpolated output samples
	unsigned int count = maxSamplingRatio;			///< number of samples in output array
};


/**
\class Interpolator
\ingroup FFTW-Objects
\brief
The Interpolator object implements a sample rate interpolator. One input sample yields N output samples.

Audio I/O:
- Processes mono input to interpoalted (multi-sample) output.

Control I/F:
- none.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class Interpolator
{
public:
	Interpolator() { }		/* C-TOR */
	~Interpolator() { }		/* D-TOR */

	/** setup the sample rate interpolator */
	/**
	\param _FIRLength the interpolator's anti-aliasing filter length
	\param _ratio the conversion ratio (see rateConversionRatio)
	\param _sampleRate the actual sample rate
	\param _polyphase flag to enable polyphase decomposition
	*/
	inline void initialize(unsigned int _FIRLength, rateConversionRatio _ratio, unsigned int _sampleRate, bool _polyphase = true)
	{
		polyphase = _polyphase;
		sampleRate = _sampleRate;
		FIRLength = _FIRLength;
		ratio = _ratio;
		unsigned int count = countForRatio(ratio);
		unsigned int subBandLength = FIRLength / count;

		// --- straight SRC, no polyphase
		convolver.initialize(FIRLength);

		// --- set filterIR from built-in set - user can always override this!
		double* filterTable = getFilterIRTable(FIRLength, ratio, sampleRate);
		if (!filterTable) return;
		convolver.setFilterIR(filterTable);

		if (!polyphase) return;

		// --- decompose filter
		double** polyPhaseFilters = decomposeFilter(filterTable, FIRLength, count);
		if (!polyPhaseFilters)
		{
			polyphase = false;
			return;
		}

		// --- set the individual polyphase filter IRs on the convolvers
		for (unsigned int i = 0; i < count; i++)
		{
			polyPhaseConvolvers[i].initialize(subBandLength);
			polyPhaseConvolvers[i].setFilterIR(polyPhaseFilters[i]);
			delete[] polyPhaseFilters[i];
		}

		delete[] polyPhaseFilters;
	}

	/** perform the interpolation; the multiple outputs are in an array in the return structure */
	inline InterpolatorOutput interpolateAudio(double xn)
	{
		unsigned int count = countForRatio(ratio);

		// --- setup output
		InterpolatorOutput output;
		output.count = count;

		// --- interpolators need the amp correction
		double ampCorrection = double(count);

		// --- polyphase uses "backwards" indexing for interpolator; see book
		int m = count-1;
		for (unsigned int i = 0; i < count; i++)
		{
			if (!polyphase)
				output.audioData[i] = i == 0 ? ampCorrection*convolver.processAudioSample(xn) : ampCorrection*convolver.processAudioSample(0.0);
			else
				output.audioData[i] = ampCorrection*polyPhaseConvolvers[m--].processAudioSample(xn);
		}
		return output;
	}

protected:
	// --- for straight, non-polyphase
	FastConvolver convolver; ///< the convolver

	// --- we save these for future expansion, currently only sparsely used
	unsigned int sampleRate = 44100;	///< sample rate
	unsigned int FIRLength = 256;		///< FIR length
	rateConversionRatio ratio = rateConversionRatio::k2x; ///< conversion ration

	// --- polyphase: 4x is max right now
	bool polyphase = true;									///< enable polyphase decomposition
	FastConvolver polyPhaseConvolvers[maxSamplingRatio];	///< a set of sub-band convolvers for polyphase operation
};

/**
\struct DecimatorInput
\ingroup FFTW-Objects
\brief
Custom input structure for DecimatorInput; it holds an arry of input samples that will be decimated down to just one sample.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct DecimatorInput
{
	DecimatorInput() {}
	double audioData[maxSamplingRatio] = { 0.0 };	///< input array of samples to be decimated
	unsigned int count = maxSamplingRatio;			///< count of samples in input array
};

/**
\class Decimator
\ingroup FFTW-Objects
\brief
The Decimator object implements a sample rate decimator. Ana array of M input samples is decimated
to one output sample.

Audio I/O:
- Processes mono input to interpoalted (multi-sample) output.

Control I/F:
- none.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class Decimator
{
public:
	Decimator() { }		/* C-TOR */
	~Decimator() { }	/* D-TOR */

	/** setup the sample rate decimator */
	/**
	\param _FIRLength the decimator's anti-aliasing filter length
	\param _ratio the conversion ratio (see rateConversionRatio)
	\param _sampleRate the actual sample rate
	\param _polyphase flag to enable polyphase decomposition
	*/
	inline void initialize(unsigned int _FIRLength, rateConversionRatio _ratio, unsigned int _sampleRate, bool _polyphase = true)
	{
		polyphase = _polyphase;
		sampleRate = _sampleRate;
		FIRLength = _FIRLength;
		ratio = _ratio;
		unsigned int count = countForRatio(ratio);
		unsigned int subBandLength = FIRLength / count;

		// --- straight SRC, no polyphase
		convolver.initialize(FIRLength);

		// --- set filterIR from built-in set - user can always override this!
		double* filterTable = getFilterIRTable(FIRLength, ratio, sampleRate);
		if (!filterTable) return;
		convolver.setFilterIR(filterTable);

		if (!polyphase) return;

		// --- decompose filter
		double** polyPhaseFilters = decomposeFilter(filterTable, FIRLength, count);
		if (!polyPhaseFilters)
		{
			polyphase = false;
			return;
		}

		// --- set the individual polyphase filter IRs on the convolvers
		for (unsigned int i = 0; i < count; i++)
		{
			polyPhaseConvolvers[i].initialize(subBandLength);
			polyPhaseConvolvers[i].setFilterIR(polyPhaseFilters[i]);
			delete[] polyPhaseFilters[i];
		}

		delete[] polyPhaseFilters;
	}

	/** decimate audio input samples into one outut sample (return value) */
	inline double decimateAudio(DecimatorInput data)
	{
		unsigned int count = countForRatio(ratio);

		// --- setup output
		double output = 0.0;

		// --- polyphase uses "forwards" indexing for decimator; see book
		for (unsigned int i = 0; i < count; i++)
		{
			if (!polyphase) // overwrites output; only the last output is saved
				output = convolver.processAudioSample(data.audioData[i]);
			else
				output += polyPhaseConvolvers[i].processAudioSample(data.audioData[i]);
		}
		return output;
	}

protected:
	// --- for straight, non-polyphase
	FastConvolver convolver;		 ///< fast convolver

	// --- we save these for future expansion, currently only sparsely used
	unsigned int sampleRate = 44100;	///< sample rate
	unsigned int FIRLength = 256;		///< FIR length
	rateConversionRatio ratio = rateConversionRatio::k2x; ///< conversion ration

	// --- polyphase: 4x is max right now
	bool polyphase = true;									///< enable polyphase decomposition
	FastConvolver polyPhaseConvolvers[maxSamplingRatio];	///< a set of sub-band convolvers for polyphase operation
};

#endif
