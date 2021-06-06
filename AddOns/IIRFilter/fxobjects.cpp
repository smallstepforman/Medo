// -----------------------------------------------------------------------------
//    ASPiK-Core File:  fxobjects.cpp
//
/**
    \file   fxobjects.cpp
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
#include <memory>
#include <math.h>
#include <cstring>
#include "fxobjects.h"

/**
\brief returns the storage component S(n) for delay-free loop solutions

- NOTES:\n
the storageComponent or "S" value is used for Zavalishin's VA filters as well
as the phaser APFs (Biquad) and is only available on two of the forms: direct
and transposed canonical\n

\returns the storage component of the filter
*/
double Biquad::getS_value()
{
	storageComponent = 0.0;
	if (parameters.biquadCalcType == biquadAlgorithm::kDirect)
	{
		// --- 1)  form output y(n) = a0*x(n) + a1*x(n-1) + a2*x(n-2) - b1*y(n-1) - b2*y(n-2)
		storageComponent = coeffArray[a1] * stateArray[x_z1] +
			coeffArray[a2] * stateArray[x_z2] -
			coeffArray[b1] * stateArray[y_z1] -
			coeffArray[b2] * stateArray[y_z2];
	}
	else if (parameters.biquadCalcType == biquadAlgorithm::kTransposeCanonical)
	{
		// --- 1)  form output y(n) = a0*x(n) + stateArray[x_z1]
		storageComponent = stateArray[x_z1];
	}

	return storageComponent;
}

/**
\brief process one sample through the biquad

- RULES:\n
1) do all math required to form the output y(n), reading registers as required - do NOT write registers \n
2) check for underflow, which can happen with feedback structures\n
3) lastly, update the states of the z^-1 registers in the state array just before returning\n

- NOTES:\n
the storageComponent or "S" value is used for Zavalishin's VA filters and is only
available on two of the forms: direct and transposed canonical\n

\param xn the input sample x(n)
\returns the biquad processed output y(n)
*/
double Biquad::processAudioSample(double xn)
{
	if (parameters.biquadCalcType == biquadAlgorithm::kDirect)
	{
		// --- 1)  form output y(n) = a0*x(n) + a1*x(n-1) + a2*x(n-2) - b1*y(n-1) - b2*y(n-2)
		double yn = coeffArray[a0] * xn + 
					coeffArray[a1] * stateArray[x_z1] +
					coeffArray[a2] * stateArray[x_z2] -
					coeffArray[b1] * stateArray[y_z1] -
					coeffArray[b2] * stateArray[y_z2];

		// --- 2) underflow check
		checkFloatUnderflow(yn);

		// --- 3) update states
		stateArray[x_z2] = stateArray[x_z1];
		stateArray[x_z1] = xn;

		stateArray[y_z2] = stateArray[y_z1];
		stateArray[y_z1] = yn;

		// --- return value
		return yn;
	}
	else if (parameters.biquadCalcType == biquadAlgorithm::kCanonical)
	{
		// --- 1)  form output y(n) = a0*w(n) + m_f_a1*stateArray[x_z1] + m_f_a2*stateArray[x_z2][x_z2];
		//
		// --- w(n) = x(n) - b1*stateArray[x_z1] - b2*stateArray[x_z2]
		double wn = xn - coeffArray[b1] * stateArray[x_z1] - coeffArray[b2] * stateArray[x_z2];

		// --- y(n):
		double yn = coeffArray[a0] * wn + coeffArray[a1] * stateArray[x_z1] + coeffArray[a2] * stateArray[x_z2];

		// --- 2) underflow check
		checkFloatUnderflow(yn);

		// --- 3) update states
		stateArray[x_z2] = stateArray[x_z1];
		stateArray[x_z1] = wn;

		// --- return value
		return yn;
	}
	else if (parameters.biquadCalcType == biquadAlgorithm::kTransposeDirect)
	{
		// --- 1)  form output y(n) = a0*w(n) + stateArray[x_z1]
		//
		// --- w(n) = x(n) + stateArray[y_z1]
		double wn = xn + stateArray[y_z1];

		// --- y(n) = a0*w(n) + stateArray[x_z1]
		double yn = coeffArray[a0] * wn + stateArray[x_z1];

		// --- 2) underflow check
		checkFloatUnderflow(yn);

		// --- 3) update states
		stateArray[y_z1] = stateArray[y_z2] - coeffArray[b1] * wn;
		stateArray[y_z2] = -coeffArray[b2] * wn;

		stateArray[x_z1] = stateArray[x_z2] + coeffArray[a1] * wn;
		stateArray[x_z2] = coeffArray[a2] * wn;

		// --- return value
		return yn;
	}
	else if (parameters.biquadCalcType == biquadAlgorithm::kTransposeCanonical)
	{
		// --- 1)  form output y(n) = a0*x(n) + stateArray[x_z1]
		double yn = coeffArray[a0] * xn + stateArray[x_z1];

		// --- 2) underflow check
		checkFloatUnderflow(yn);

		// --- shuffle/update
		stateArray[x_z1] = coeffArray[a1]*xn - coeffArray[b1]*yn + stateArray[x_z2];
		stateArray[x_z2] = coeffArray[a2]*xn - coeffArray[b2]*yn;

		// --- return value
		return yn;
	}
	return xn; // didn't process anything :(
}

// --- returns true if coeffs were updated
bool AudioFilter::calculateFilterCoeffs()
{
	// --- clear coeff array
	memset(&coeffArray[0], 0, sizeof(double)*numCoeffs);

	// --- set default pass-through
	coeffArray[a0] = 1.0;
	coeffArray[c0] = 1.0;
	coeffArray[d0] = 0.0;

	// --- grab these variables, to make calculations look more like the book
	filterAlgorithm algorithm = audioFilterParameters.algorithm;
	double fc = audioFilterParameters.fc;
	double Q = audioFilterParameters.Q;
	double boostCut_dB = audioFilterParameters.boostCut_dB;

	// --- decode filter type and calculate accordingly
	// --- impulse invariabt LPF, matches closely with one-pole version,
	//     but diverges at VHF
	if (algorithm == filterAlgorithm::kImpInvLP1)
	{
		double T = 1.0 / sampleRate;
		double omega = 2.0*kPi*fc;
		double eT = exp(-T*omega);

		coeffArray[a0] = 1.0 - eT; // <--- normalized by 1-e^aT
		coeffArray[a1] = 0.0;
		coeffArray[a2] = 0.0;
		coeffArray[b1] = -eT;
		coeffArray[b2] = 0.0;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;

	}
	else if (algorithm == filterAlgorithm::kImpInvLP2)
	{
		double alpha = 2.0*kPi*fc / sampleRate;
		double p_Re = -alpha / (2.0*Q);
		double zeta = 1.0 / (2.0 * Q);
		double p_Im = alpha*pow((1.0 - (zeta*zeta)), 0.5);
		double c_Re = 0.0;
		double c_Im = alpha / (2.0*pow((1.0 - (zeta*zeta)), 0.5));

		double eP_re = exp(p_Re);
		coeffArray[a0] = c_Re;
		coeffArray[a1] = -2.0*(c_Re*cos(p_Im) + c_Im*sin(p_Im))*exp(p_Re);
		coeffArray[a2] = 0.0;
		coeffArray[b1] = -2.0*eP_re*cos(p_Im);
		coeffArray[b2] = eP_re*eP_re;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	// --- kMatchLP2A = TIGHT fit LPF vicanek algo
	else if (algorithm == filterAlgorithm::kMatchLP2A)
	{
		// http://vicanek.de/articles/BiquadFits.pdf
		double theta_c = 2.0*kPi*fc / sampleRate;

		double q = 1.0 / (2.0*Q);

		// --- impulse invariant
		double b_1 = 0.0;
		double b_2 = exp(-2.0*q*theta_c);
		if (q <= 1.0)
		{
			b_1 = -2.0*exp(-q*theta_c)*cos(pow((1.0 - q*q), 0.5)*theta_c);
		}
		else
		{
			b_1 = -2.0*exp(-q*theta_c)*cosh(pow((q*q - 1.0), 0.5)*theta_c);
		}

		// --- TIGHT FIT --- //
		double B0 = (1.0 + b_1 + b_2)*(1.0 + b_1 + b_2);
		double B1 = (1.0 - b_1 + b_2)*(1.0 - b_1 + b_2);
		double B2 = -4.0*b_2;

		double phi_0 = 1.0 - sin(theta_c / 2.0)*sin(theta_c / 2.0);
		double phi_1 = sin(theta_c / 2.0)*sin(theta_c / 2.0);
		double phi_2 = 4.0*phi_0*phi_1;

		double R1 = (B0*phi_0 + B1*phi_1 + B2*phi_2)*(Q*Q);
		double A0 = B0;
		double A1 = (R1 - A0*phi_0) / phi_1;

		if (A0 < 0.0)
			A0 = 0.0;
		if (A1 < 0.0)
			A1 = 0.0;

		double a_0 = 0.5*(pow(A0, 0.5) + pow(A1, 0.5));
		double a_1 = pow(A0, 0.5) - a_0;
		double a_2 = 0.0;

		coeffArray[a0] = a_0;
		coeffArray[a1] = a_1;
		coeffArray[a2] = a_2;
		coeffArray[b1] = b_1;
		coeffArray[b2] = b_2;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	// --- kMatchLP2B = LOOSE fit LPF vicanek algo
	else if (algorithm == filterAlgorithm::kMatchLP2B)
	{
		// http://vicanek.de/articles/BiquadFits.pdf
		double theta_c = 2.0*kPi*fc / sampleRate;
		double q = 1.0 / (2.0*Q);

		// --- impulse invariant
		double b_1 = 0.0;
		double b_2 = exp(-2.0*q*theta_c);
		if (q <= 1.0)
		{
			b_1 = -2.0*exp(-q*theta_c)*cos(pow((1.0 - q*q), 0.5)*theta_c);
		}
		else
		{
			b_1 = -2.0*exp(-q*theta_c)*cosh(pow((q*q - 1.0), 0.5)*theta_c);
		}

		// --- LOOSE FIT --- //
		double f0 = theta_c / kPi; // note f0 = fraction of pi, so that f0 = 1.0 = pi = Nyquist

		double r0 = 1.0 + b_1 + b_2;
		double denom = (1.0 - f0*f0)*(1.0 - f0*f0) + (f0*f0) / (Q*Q);
		denom = pow(denom, 0.5);
		double r1 = ((1.0 - b_1 + b_2)*f0*f0) / (denom);

		double a_0 = (r0 + r1) / 2.0;
		double a_1 = r0 - a_0;
		double a_2 = 0.0;

		coeffArray[a0] = a_0;
		coeffArray[a1] = a_1;
		coeffArray[a2] = a_2;
		coeffArray[b1] = b_1;
		coeffArray[b2] = b_2;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	// --- kMatchBP2A = TIGHT fit BPF vicanek algo
	else if (algorithm == filterAlgorithm::kMatchBP2A)
	{
		// http://vicanek.de/articles/BiquadFits.pdf
		double theta_c = 2.0*kPi*fc / sampleRate;
		double q = 1.0 / (2.0*Q);

		// --- impulse invariant
		double b_1 = 0.0;
		double b_2 = exp(-2.0*q*theta_c);
		if (q <= 1.0)
		{
			b_1 = -2.0*exp(-q*theta_c)*cos(pow((1.0 - q*q), 0.5)*theta_c);
		}
		else
		{
			b_1 = -2.0*exp(-q*theta_c)*cosh(pow((q*q - 1.0), 0.5)*theta_c);
		}

		// --- TIGHT FIT --- //
		double B0 = (1.0 + b_1 + b_2)*(1.0 + b_1 + b_2);
		double B1 = (1.0 - b_1 + b_2)*(1.0 - b_1 + b_2);
		double B2 = -4.0*b_2;

		double phi_0 = 1.0 - sin(theta_c / 2.0)*sin(theta_c / 2.0);
		double phi_1 = sin(theta_c / 2.0)*sin(theta_c / 2.0);
		double phi_2 = 4.0*phi_0*phi_1;

		double R1 = B0*phi_0 + B1*phi_1 + B2*phi_2;
		double R2 = -B0 + B1 + 4.0*(phi_0 - phi_1)*B2;

		double A2 = (R1 - R2*phi_1) / (4.0*phi_1*phi_1);
		double A1 = R2 + 4.0*(phi_1 - phi_0)*A2;

		double a_1 = -0.5*(pow(A1, 0.5));
		double a_0 = 0.5*(pow((A2 + (a_1*a_1)), 0.5) - a_1);
		double a_2 = -a_0 - a_1;

		coeffArray[a0] = a_0;
		coeffArray[a1] = a_1;
		coeffArray[a2] = a_2;
		coeffArray[b1] = b_1;
		coeffArray[b2] = b_2;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	// --- kMatchBP2B = LOOSE fit BPF vicanek algo
	else if (algorithm == filterAlgorithm::kMatchBP2B)
	{
		// http://vicanek.de/articles/BiquadFits.pdf
		double theta_c = 2.0*kPi*fc / sampleRate;
		double q = 1.0 / (2.0*Q);

		// --- impulse invariant
		double b_1 = 0.0;
		double b_2 = exp(-2.0*q*theta_c);
		if (q <= 1.0)
		{
			b_1 = -2.0*exp(-q*theta_c)*cos(pow((1.0 - q*q), 0.5)*theta_c);
		}
		else
		{
			b_1 = -2.0*exp(-q*theta_c)*cosh(pow((q*q - 1.0), 0.5)*theta_c);
		}

		// --- LOOSE FIT --- //
		double f0 = theta_c / kPi; // note f0 = fraction of pi, so that f0 = 1.0 = pi = Nyquist

		double r0 = (1.0 + b_1 + b_2) / (kPi*f0*Q);
		double denom = (1.0 - f0*f0)*(1.0 - f0*f0) + (f0*f0) / (Q*Q);
		denom = pow(denom, 0.5);

		double r1 = ((1.0 - b_1 + b_2)*(f0 / Q)) / (denom);

		double a_1 = -r1 / 2.0;
		double a_0 = (r0 - a_1) / 2.0;
		double a_2 = -a_0 - a_1;

		coeffArray[a0] = a_0;
		coeffArray[a1] = a_1;
		coeffArray[a2] = a_2;
		coeffArray[b1] = b_1;
		coeffArray[b2] = b_2;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kLPF1P)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double gamma = 2.0 - cos(theta_c);

		double filter_b1 = pow((gamma*gamma - 1.0), 0.5) - gamma;
		double filter_a0 = 1.0 + filter_b1;

		// --- update coeffs
		coeffArray[a0] = filter_a0;
		coeffArray[a1] = 0.0;
		coeffArray[a2] = 0.0;
		coeffArray[b1] = filter_b1;
		coeffArray[b2] = 0.0;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kLPF1)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double gamma = cos(theta_c) / (1.0 + sin(theta_c));

		// --- update coeffs
		coeffArray[a0] = (1.0 - gamma) / 2.0;
		coeffArray[a1] = (1.0 - gamma) / 2.0;
		coeffArray[a2] = 0.0;
		coeffArray[b1] = -gamma;
		coeffArray[b2] = 0.0;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kHPF1)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double gamma = cos(theta_c) / (1.0 + sin(theta_c));

		// --- update coeffs
		coeffArray[a0] = (1.0 + gamma) / 2.0;
		coeffArray[a1] = -(1.0 + gamma) / 2.0;
		coeffArray[a2] = 0.0;
		coeffArray[b1] = -gamma;
		coeffArray[b2] = 0.0;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kLPF2)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double d = 1.0 / Q;
		double betaNumerator = 1.0 - ((d / 2.0)*(sin(theta_c)));
		double betaDenominator = 1.0 + ((d / 2.0)*(sin(theta_c)));

		double beta = 0.5*(betaNumerator / betaDenominator);
		double gamma = (0.5 + beta)*(cos(theta_c));
		double alpha = (0.5 + beta - gamma) / 2.0;

		// --- update coeffs
		coeffArray[a0] = alpha;
		coeffArray[a1] = 2.0*alpha;
		coeffArray[a2] = alpha;
		coeffArray[b1] = -2.0*gamma;
		coeffArray[b2] = 2.0*beta;

		double mag = getMagResponse(theta_c, coeffArray[a0], coeffArray[a1], coeffArray[a2], coeffArray[b1], coeffArray[b2]);
		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kHPF2)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double d = 1.0 / Q;

		double betaNumerator = 1.0 - ((d / 2.0)*(sin(theta_c)));
		double betaDenominator = 1.0 + ((d / 2.0)*(sin(theta_c)));

		double beta = 0.5*(betaNumerator / betaDenominator);
		double gamma = (0.5 + beta)*(cos(theta_c));
		double alpha = (0.5 + beta + gamma) / 2.0;

		// --- update coeffs
		coeffArray[a0] = alpha;
		coeffArray[a1] = -2.0*alpha;
		coeffArray[a2] = alpha;
		coeffArray[b1] = -2.0*gamma;
		coeffArray[b2] = 2.0*beta;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kBPF2)
	{
		// --- see book for formulae
		double K = tan(kPi*fc / sampleRate);
		double delta = K*K*Q + K + Q;

		// --- update coeffs
		coeffArray[a0] = K / delta;;
		coeffArray[a1] = 0.0;
		coeffArray[a2] = -K / delta;
		coeffArray[b1] = 2.0*Q*(K*K - 1) / delta;
		coeffArray[b2] = (K*K*Q - K + Q) / delta;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kBSF2)
	{
		// --- see book for formulae
		double K = tan(kPi*fc / sampleRate);
		double delta = K*K*Q + K + Q;

		// --- update coeffs
		coeffArray[a0] = Q*(1 + K*K) / delta;
		coeffArray[a1] = 2.0*Q*(K*K - 1) / delta;
		coeffArray[a2] = Q*(1 + K*K) / delta;
		coeffArray[b1] = 2.0*Q*(K*K - 1) / delta;
		coeffArray[b2] = (K*K*Q - K + Q) / delta;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kButterLPF2)
	{
		// --- see book for formulae
		double theta_c = kPi*fc / sampleRate;
		double C = 1.0 / tan(theta_c);

		// --- update coeffs
		coeffArray[a0] = 1.0 / (1.0 + kSqrtTwo*C + C*C);
		coeffArray[a1] = 2.0*coeffArray[a0];
		coeffArray[a2] = coeffArray[a0];
		coeffArray[b1] = 2.0*coeffArray[a0] * (1.0 - C*C);
		coeffArray[b2] = coeffArray[a0] * (1.0 - kSqrtTwo*C + C*C);

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kButterHPF2)
	{
		// --- see book for formulae
		double theta_c = kPi*fc / sampleRate;
		double C = tan(theta_c);

		// --- update coeffs
		coeffArray[a0] = 1.0 / (1.0 + kSqrtTwo*C + C*C);
		coeffArray[a1] = -2.0*coeffArray[a0];
		coeffArray[a2] = coeffArray[a0];
		coeffArray[b1] = 2.0*coeffArray[a0] * (C*C - 1.0);
		coeffArray[b2] = coeffArray[a0] * (1.0 - kSqrtTwo*C + C*C);

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kButterBPF2)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double BW = fc / Q;
		double delta_c = kPi*BW / sampleRate;
		if (delta_c >= 0.95*kPi / 2.0) delta_c = 0.95*kPi / 2.0;

		double C = 1.0 / tan(delta_c);
		double D = 2.0*cos(theta_c);

		// --- update coeffs
		coeffArray[a0] = 1.0 / (1.0 + C);
		coeffArray[a1] = 0.0;
		coeffArray[a2] = -coeffArray[a0];
		coeffArray[b1] = -coeffArray[a0] * (C*D);
		coeffArray[b2] = coeffArray[a0] * (C - 1.0);

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kButterBSF2)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double BW = fc / Q;
		double delta_c = kPi*BW / sampleRate;
		if (delta_c >= 0.95*kPi / 2.0) delta_c = 0.95*kPi / 2.0;

		double C = tan(delta_c);
		double D = 2.0*cos(theta_c);

		// --- update coeffs
		coeffArray[a0] = 1.0 / (1.0 + C);
		coeffArray[a1] = -coeffArray[a0] * D;
		coeffArray[a2] = coeffArray[a0];
		coeffArray[b1] = -coeffArray[a0] * D;
		coeffArray[b2] = coeffArray[a0] * (1.0 - C);

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kMMALPF2 || algorithm == filterAlgorithm::kMMALPF2B)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double resonance_dB = 0;

		if (Q > 0.707)
		{
			double peak = Q*Q / pow(Q*Q - 0.25, 0.5);
			resonance_dB = 20.0*log10(peak);
		}

		// --- intermediate vars
		double resonance = (cos(theta_c) + (sin(theta_c) * sqrt(pow(10.0, (resonance_dB / 10.0)) - 1))) / ((pow(10.0, (resonance_dB / 20.0)) * sin(theta_c)) + 1);
		double g = pow(10.0, (-resonance_dB / 40.0));

		// --- kMMALPF2B disables the GR with increase in Q
		if (algorithm == filterAlgorithm::kMMALPF2B)
			g = 1.0;

		double filter_b1 = (-2.0) * resonance * cos(theta_c);
		double filter_b2 = resonance * resonance;
		double filter_a0 = g * (1 + filter_b1 + filter_b2);

		// --- update coeffs
		coeffArray[a0] = filter_a0;
		coeffArray[a1] = 0.0;
		coeffArray[a2] = 0.0;
		coeffArray[b1] = filter_b1;
		coeffArray[b2] = filter_b2;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kLowShelf)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double mu = pow(10.0, boostCut_dB / 20.0);

		double beta = 4.0 / (1.0 + mu);
		double delta = beta*tan(theta_c / 2.0);
		double gamma = (1.0 - delta) / (1.0 + delta);

		// --- update coeffs
		coeffArray[a0] = (1.0 - gamma) / 2.0;
		coeffArray[a1] = (1.0 - gamma) / 2.0;
		coeffArray[a2] = 0.0;
		coeffArray[b1] = -gamma;
		coeffArray[b2] = 0.0;

		coeffArray[c0] = mu - 1.0;
		coeffArray[d0] = 1.0;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kHiShelf)
	{
		double theta_c = 2.0*kPi*fc / sampleRate;
		double mu = pow(10.0, boostCut_dB / 20.0);

		double beta = (1.0 + mu) / 4.0;
		double delta = beta*tan(theta_c / 2.0);
		double gamma = (1.0 - delta) / (1.0 + delta);

		coeffArray[a0] = (1.0 + gamma) / 2.0;
		coeffArray[a1] = -coeffArray[a0];
		coeffArray[a2] = 0.0;
		coeffArray[b1] = -gamma;
		coeffArray[b2] = 0.0;

		coeffArray[c0] = mu - 1.0;
		coeffArray[d0] = 1.0;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kCQParaEQ)
	{
		// --- see book for formulae
		double K = tan(kPi*fc / sampleRate);
		double Vo = pow(10.0, boostCut_dB / 20.0);
		bool bBoost = boostCut_dB >= 0 ? true : false;

		double d0 = 1.0 + (1.0 / Q)*K + K*K;
		double e0 = 1.0 + (1.0 / (Vo*Q))*K + K*K;
		double alpha = 1.0 + (Vo / Q)*K + K*K;
		double beta = 2.0*(K*K - 1.0);
		double gamma = 1.0 - (Vo / Q)*K + K*K;
		double delta = 1.0 - (1.0 / Q)*K + K*K;
		double eta = 1.0 - (1.0 / (Vo*Q))*K + K*K;

		// --- update coeffs
		coeffArray[a0] = bBoost ? alpha / d0 : d0 / e0;
		coeffArray[a1] = bBoost ? beta / d0 : beta / e0;
		coeffArray[a2] = bBoost ? gamma / d0 : delta / e0;
		coeffArray[b1] = bBoost ? beta / d0 : beta / e0;
		coeffArray[b2] = bBoost ? delta / d0 : eta / e0;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kNCQParaEQ)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double mu = pow(10.0, boostCut_dB / 20.0);

		// --- clamp to 0.95 pi/2 (you can experiment with this)
		double tanArg = theta_c / (2.0 * Q);
		if (tanArg >= 0.95*kPi / 2.0) tanArg = 0.95*kPi / 2.0;

		// --- intermediate variables (you can condense this if you wish)
		double zeta = 4.0 / (1.0 + mu);
		double betaNumerator = 1.0 - zeta*tan(tanArg);
		double betaDenominator = 1.0 + zeta*tan(tanArg);

		double beta = 0.5*(betaNumerator / betaDenominator);
		double gamma = (0.5 + beta)*(cos(theta_c));
		double alpha = (0.5 - beta);

		// --- update coeffs
		coeffArray[a0] = alpha;
		coeffArray[a1] = 0.0;
		coeffArray[a2] = -alpha;
		coeffArray[b1] = -2.0*gamma;
		coeffArray[b2] = 2.0*beta;

		coeffArray[c0] = mu - 1.0;
		coeffArray[d0] = 1.0;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kLWRLPF2)
	{
		// --- see book for formulae
		double omega_c = kPi*fc;
		double theta_c = kPi*fc / sampleRate;

		double k = omega_c / tan(theta_c);
		double denominator = k*k + omega_c*omega_c + 2.0*k*omega_c;
		double b1_Num = -2.0*k*k + 2.0*omega_c*omega_c;
		double b2_Num = -2.0*k*omega_c + k*k + omega_c*omega_c;

		// --- update coeffs
		coeffArray[a0] = omega_c*omega_c / denominator;
		coeffArray[a1] = 2.0*omega_c*omega_c / denominator;
		coeffArray[a2] = coeffArray[a0];
		coeffArray[b1] = b1_Num / denominator;
		coeffArray[b2] = b2_Num / denominator;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kLWRHPF2)
	{
		// --- see book for formulae
		double omega_c = kPi*fc;
		double theta_c = kPi*fc / sampleRate;

		double k = omega_c / tan(theta_c);
		double denominator = k*k + omega_c*omega_c + 2.0*k*omega_c;
		double b1_Num = -2.0*k*k + 2.0*omega_c*omega_c;
		double b2_Num = -2.0*k*omega_c + k*k + omega_c*omega_c;

		// --- update coeffs
		coeffArray[a0] = k*k / denominator;
		coeffArray[a1] = -2.0*k*k / denominator;
		coeffArray[a2] = coeffArray[a0];
		coeffArray[b1] = b1_Num / denominator;
		coeffArray[b2] = b2_Num / denominator;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kAPF1)
	{
		// --- see book for formulae
		double alphaNumerator = tan((kPi*fc) / sampleRate) - 1.0;
		double alphaDenominator = tan((kPi*fc) / sampleRate) + 1.0;
		double alpha = alphaNumerator / alphaDenominator;

		// --- update coeffs
		coeffArray[a0] = alpha;
		coeffArray[a1] = 1.0;
		coeffArray[a2] = 0.0;
		coeffArray[b1] = alpha;
		coeffArray[b2] = 0.0;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kAPF2)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double BW = fc / Q;
		double argTan = kPi*BW / sampleRate;
		if (argTan >= 0.95*kPi / 2.0) argTan = 0.95*kPi / 2.0;

		double alphaNumerator = tan(argTan) - 1.0;
		double alphaDenominator = tan(argTan) + 1.0;
		double alpha = alphaNumerator / alphaDenominator;
		double beta = -cos(theta_c);

		// --- update coeffs
		coeffArray[a0] = -alpha;
		coeffArray[a1] = beta*(1.0 - alpha);
		coeffArray[a2] = 1.0;
		coeffArray[b1] = beta*(1.0 - alpha);
		coeffArray[b2] = -alpha;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kResonA)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double BW = fc / Q;
		double filter_b2 = exp(-2.0*kPi*(BW / sampleRate));
		double filter_b1 = ((-4.0*filter_b2) / (1.0 + filter_b2))*cos(theta_c);
		double filter_a0 = (1.0 - filter_b2)*pow((1.0 - (filter_b1*filter_b1) / (4.0 * filter_b2)), 0.5);

		// --- update coeffs
		coeffArray[a0] = filter_a0;
		coeffArray[a1] = 0.0;
		coeffArray[a2] = 0.0;
		coeffArray[b1] = filter_b1;
		coeffArray[b2] = filter_b2;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}
	else if (algorithm == filterAlgorithm::kResonB)
	{
		// --- see book for formulae
		double theta_c = 2.0*kPi*fc / sampleRate;
		double BW = fc / Q;
		double filter_b2 = exp(-2.0*kPi*(BW / sampleRate));
		double filter_b1 = ((-4.0*filter_b2) / (1.0 + filter_b2))*cos(theta_c);
		double filter_a0 = 1.0 - pow(filter_b2, 0.5); // (1.0 - filter_b2)*pow((1.0 - (filter_b1*filter_b1) / (4.0 * filter_b2)), 0.5);

		// --- update coeffs
		coeffArray[a0] = filter_a0;
		coeffArray[a1] = 0.0;
		coeffArray[a2] = -filter_a0;
		coeffArray[b1] = filter_b1;
		coeffArray[b2] = filter_b2;

		// --- update on calculator
		biquad.setCoefficients(coeffArray);

		// --- we updated
		return true;
	}

	// --- we did n't update :(
	return false;
}

/**
\brief process one sample through the audio filter

- NOTES:\n
Uses the modified biquaqd structure that includes the wet and dry signal coefficients c and d.\n
Here the biquad object does all of the work and we simply combine the wet and dry signals.\n
// return (dry) + (processed): x(n)*d0 + y(n)*c0\n

\param xn the input sample x(n)
\returns the biquad processed output y(n)
*/
double AudioFilter::processAudioSample(double xn)
{
	// --- let biquad do the grunt-work
	//
	// return (dry) + (processed): x(n)*d0 + y(n)*c0
	return coeffArray[d0] * xn + coeffArray[c0] * biquad.processAudioSample(xn);
}

/**
\brief sets the new attack time and re-calculates the time constant

\param attack_in_ms the new attack timme
\param forceCalc flag to force a re-calculation of time constant even if values have not changed.
*/
void AudioDetector::setAttackTime(double attack_in_ms, bool forceCalc)
{
	if (!forceCalc && audioDetectorParameters.attackTime_mSec == attack_in_ms)
		return;

	audioDetectorParameters.attackTime_mSec = attack_in_ms;
	attackTime = exp(TLD_AUDIO_ENVELOPE_ANALOG_TC / (attack_in_ms * sampleRate * 0.001));
}


/**
\brief sets the new release time and re-calculates the time constant

\param release_in_ms the new relase timme
\param forceCalc flag to force a re-calculation of time constant even if values have not changed.
*/
void AudioDetector::setReleaseTime(double release_in_ms, bool forceCalc)
{
	if (!forceCalc && audioDetectorParameters.releaseTime_mSec == release_in_ms)
		return;

	audioDetectorParameters.releaseTime_mSec = release_in_ms;
	releaseTime = exp(TLD_AUDIO_ENVELOPE_ANALOG_TC / (release_in_ms * sampleRate * 0.001));
}

/**
\brief generates the oscillator output for one sample interval; note that there are multiple outputs.
*/
const SignalGenData LFO::renderAudioOutput()
{
	// --- always first!
	checkAndWrapModulo(modCounter, phaseInc);

	// --- QP output always follows location of current modulo; first set equal
	modCounterQP = modCounter;

	// --- then, advance modulo by quadPhaseInc = 0.25 = 90 degrees, AND wrap if needed
	advanceAndCheckWrapModulo(modCounterQP, 0.25);

	SignalGenData output;
	generatorWaveform waveform = lfoParameters.waveform;

	// --- calculate the oscillator value
	if (waveform == generatorWaveform::kSin)
	{
		// --- calculate normal angle
		double angle = modCounter*2.0*kPi - kPi;

		// --- norm output with parabolicSine approximation
		output.normalOutput = parabolicSine(-angle);

		// --- calculate QP angle
		angle = modCounterQP*2.0*kPi - kPi;

		// --- calc QP output
		output.quadPhaseOutput_pos = parabolicSine(-angle);
	}
	else if (waveform == generatorWaveform::kTriangle)
	{
		// triv saw
		output.normalOutput = unipolarToBipolar(modCounter);

		// bipolar triagle
		output.normalOutput = 2.0*fabs(output.normalOutput) - 1.0;

		// -- quad phase
		output.quadPhaseOutput_pos = unipolarToBipolar(modCounterQP);

		// bipolar triagle
		output.quadPhaseOutput_pos = 2.0*fabs(output.quadPhaseOutput_pos) - 1.0;
	}
	else if (waveform == generatorWaveform::kSaw)
	{
		output.normalOutput = unipolarToBipolar(modCounter);
		output.quadPhaseOutput_pos = unipolarToBipolar(modCounterQP);
	}

	// --- invert two main outputs to make the opposite versions
	output.quadPhaseOutput_neg = -output.quadPhaseOutput_pos;
	output.invertedOutput = -output.normalOutput;

	// --- setup for next sample period
	advanceModulo(modCounter, phaseInc);

	return output;
}


#ifdef HAVE_FFTW

/**
\brief destroys the FFTW arrays and plans.
*/
void FastFFT::destroyFFTW()
{
#ifdef HAVE_FFTW
	if (plan_forward)
		fftw_destroy_plan(plan_forward);
	if (plan_backward)
		fftw_destroy_plan(plan_backward);

	if (fft_input)
		fftw_free(fft_input);
	if (fft_result)
		fftw_free(fft_result);

	if (ifft_input)
		fftw_free(ifft_input);
	if (ifft_result)
		fftw_free(ifft_result);
#endif
}


/**
\brief initialize the Fast FFT object for operation

- NOTES:<br>
See notes on symmetrical window arrays in comments.<br>

\param _frameLength the FFT length - MUST be a power of 2
\param _window the window type (note: may be set to windowType::kNone)

*/
void FastFFT::initialize(unsigned int _frameLength, windowType _window)
{
	frameLength = _frameLength;
	window = _window;
	windowGainCorrection = 0.0;

	if (windowBuffer)
		delete windowBuffer;

	windowBuffer = new double[frameLength];
	memset(&windowBuffer[0], 0, frameLength * sizeof(double));


	// --- this is from Reiss & McPherson's code
	//     https://code.soundsoftware.ac.uk/projects/audio_effects_textbook_code/repository/entry/effects/pvoc_passthrough/Source/PluginProcessor.cpp
	// NOTE:	"Window functions are typically defined to be symmetrical. This will cause a
	//			problem in the overlap-add process: the windows instead need to be periodic
	//			when arranged end-to-end. As a result we calculate the window of one sample
	//			larger than usual, and drop the last sample. (This works as long as N is even.)
	//			See Julius Smith, "Spectral Audio Signal Processing" for details.
	// --- WP: this is why denominators are (frameLength) rather than (frameLength - 1)
	if (window == windowType::kRectWindow)
	{
		for (int n = 0; n < frameLength - 1; n++)
		{
			windowBuffer[n] = 1.0;
			windowGainCorrection += windowBuffer[n];
		}
	}
	else if (window == windowType::kHammingWindow)
	{
		for (int n = 0; n < frameLength - 1; n++)
		{
			windowBuffer[n] = 0.54 - 0.46*cos((n*2.0*kPi) / (frameLength));
			windowGainCorrection += windowBuffer[n];
		}
	}
	else if (window == windowType::kHannWindow)
	{
		for (int n = 0; n < frameLength; n++)
		{
			windowBuffer[n] = 0.5 * (1 - cos((n*2.0*kPi) / (frameLength)));
			windowGainCorrection += windowBuffer[n];
		}
	}
	else if (window == windowType::kBlackmanHarrisWindow)
	{
		for (int n = 0; n < frameLength; n++)
		{
			windowBuffer[n] = (0.42323 - (0.49755*cos((n*2.0*kPi) / (frameLength))) + 0.07922*cos((2 * n*2.0*kPi) / (frameLength)));
			windowGainCorrection += windowBuffer[n];
		}
	}
	else if (window == windowType::kNoWindow)
	{
		for (int n = 0; n < frameLength; n++)
		{
			windowBuffer[n] = 1.0;
			windowGainCorrection += windowBuffer[n];
		}
	}
	else // --- default to kNoWindow
	{
		for (int n = 0; n < frameLength; n++)
		{
			windowBuffer[n] = 1.0;
			windowGainCorrection += windowBuffer[n];
		}
	}

	// --- calculate gain correction factor
	windowGainCorrection = 1.0 / windowGainCorrection;

	destroyFFTW();
	fft_input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * frameLength);
	fft_result = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * frameLength);

	ifft_input =  (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * frameLength);
	ifft_result = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * frameLength);

	plan_forward = fftw_plan_dft_1d(frameLength, fft_input, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);
	plan_backward = fftw_plan_dft_1d(frameLength, ifft_input, ifft_result, FFTW_BACKWARD, FFTW_ESTIMATE);
}

/**
\brief perform the FFT operation

- NOTES:<br>

\param inputReal an array of real valued points
\param inputImag an array of imaginary valued points (will be 0 for audio which is real-valued)

\returns a pointer to a fftw_complex array: a 2D array of real (column 0) and imaginary (column 1) parts
*/
fftw_complex* FastFFT::doFFT(double* inputReal, double* inputImag)
{
	// ------ load up the FFT input array
	for (int i = 0; i < frameLength; i++)
	{
		fft_input[i][0] = inputReal[i];		// --- real
		if (inputImag)
			fft_input[i][1] = inputImag[i]; // --- imag
		else
			fft_input[i][1] = 0.0;
	}

	// --- do the FFT
	fftw_execute(plan_forward);

	return fft_result;
}

/**
\brief perform the IFFT operation

- NOTES:<br>

\param inputReal an array of real valued points
\param inputImag an array of imaginary valued points (will be 0 for audio which is real-valued)

\returns a pointer to a fftw_complex array: a 2D array of real (column 0) and imaginary (column 1) parts
*/
fftw_complex* FastFFT::doInverseFFT(double* inputReal, double* inputImag)
{
	// ------ load up the iFFT input array
	for (int i = 0; i < frameLength; i++)
	{
		ifft_input[i][0] = inputReal[i];		// --- real
		if (inputImag)
			ifft_input[i][1] = inputImag[i]; // --- imag
		else
			ifft_input[i][1] = 0.0;
	}

	// --- do the IFFT
	fftw_execute(plan_backward);

	return ifft_result;
}

/**
\brief destroys the FFTW arrays and plans.
*/
void PhaseVocoder::destroyFFTW()
{
	if (plan_forward)
		fftw_destroy_plan(plan_forward);
	if (plan_backward)
		fftw_destroy_plan(plan_backward);

	if (fft_input)
		fftw_free(fft_input);
	if (fft_result)
		fftw_free(fft_result);
	if (ifft_result)
		fftw_free(ifft_result);
}

/**
\brief initialize the Fast FFT object for operation

- NOTES:<br>
See notes on symmetrical window arrays in comments.<br>

\param _frameLength the FFT length - MUST be a power of 2
\param _hopSize the hop size in samples: this object only supports ha = hs (pure real-time operation only)
\param _window the window type (note: may be set to windowType::kNoWindow)

*/
void PhaseVocoder::initialize(unsigned int _frameLength, unsigned int _hopSize, windowType _window)
{
	frameLength = _frameLength;
	wrapMask = frameLength - 1;
	hopSize = _hopSize;
	window = _window;

	// --- this is the overlap as a fraction i.e. 0.75 = 75%
	overlap = hopSize > 0.0 ? 1.0 - (double)hopSize / (double)frameLength : 0.0;

	// --- gain correction for window + hop size
	windowHopCorrection = 0.0;

	// --- SETUP BUFFERS ---- //
	//     NOTE: input and output buffers are circular, others are linear
	//
	// --- input buffer, for processing the x(n) timeline
	if (inputBuffer)
		delete inputBuffer;

	inputBuffer = new double[frameLength];
	memset(&inputBuffer[0], 0, frameLength * sizeof(double));

	// --- output buffer, for processing the y(n) timeline and accumulating frames
	if (outputBuffer)
		delete outputBuffer;

	// --- the output buffer is declared as 2x the normal frame size
	//     to accomodate time-stretching/pitch shifting; you can increase the size
	//     here; if so make sure to calculate the wrapMaskOut properly and everything
	//     will work normally you can even dynamically expand and contract the buffer
	//     (not sure why you would do this - and it will surely affect CPU performance)
	//     NOTE: the length of the buffer is only to accomodate accumulations
	//           it does not stretch time or change causality on its own
	outputBuffer = new double[frameLength * 4];
	memset(&outputBuffer[0], 0, (frameLength*4.0) * sizeof(double));
	wrapMaskOut = (frameLength*4.0) - 1;

	// --- fixed window buffer
	if (windowBuffer)
		delete windowBuffer;

	windowBuffer = new double[frameLength];
	memset(&windowBuffer[0], 0, frameLength * sizeof(double));

	// --- this is from Reiss & McPherson's code
	//     https://code.soundsoftware.ac.uk/projects/audio_effects_textbook_code/repository/entry/effects/pvoc_passthrough/Source/PluginProcessor.cpp
	// NOTE:	"Window functions are typically defined to be symmetrical. This will cause a
	//			problem in the overlap-add process: the windows instead need to be periodic
	//			when arranged end-to-end. As a result we calculate the window of one sample
	//			larger than usual, and drop the last sample. (This works as long as N is even.)
	//			See Julius Smith, "Spectral Audio Signal Processing" for details.
	// --- WP: this is why denominators are (frameLength) rather than (frameLength - 1)
	if (window == windowType::kRectWindow)
	{
		for (int n = 0; n < frameLength - 1; n++)
		{
			windowBuffer[n] = 1.0;
			windowHopCorrection += windowBuffer[n];
		}
	}
	else if (window == windowType::kHammingWindow)
	{
		for (int n = 0; n < frameLength - 1; n++)
		{
			windowBuffer[n] = 0.54 - 0.46*cos((n*2.0*kPi) / (frameLength));
			windowHopCorrection += windowBuffer[n];
		}
	}
	else if (window == windowType::kHannWindow)
	{
		for (int n = 0; n < frameLength; n++)
		{
			windowBuffer[n] = 0.5 * (1 - cos((n*2.0*kPi) / (frameLength)));
			windowHopCorrection += windowBuffer[n];
		}
	}
	else if (window == windowType::kBlackmanHarrisWindow)
	{
		for (int n = 0; n < frameLength; n++)
		{
			windowBuffer[n] = (0.42323 - (0.49755*cos((n*2.0*kPi) / (frameLength))) + 0.07922*cos((2 * n*2.0*kPi) / (frameLength)));
			windowHopCorrection += windowBuffer[n];
		}
	}
	else if (window == windowType::kNoWindow)
	{
		for (int n = 0; n < frameLength; n++)
		{
			windowBuffer[n] = 1.0;
			windowHopCorrection += windowBuffer[n];
		}
	}
	else // --- default to kNoWindow
	{
		for (int n = 0; n < frameLength; n++)
		{
			windowBuffer[n] = 1.0;
			windowHopCorrection += windowBuffer[n];
		}
	}

	// --- calculate gain correction factor
	if (window != windowType::kNoWindow)
		windowHopCorrection = (1.0 - overlap) / windowHopCorrection;
	else
		windowHopCorrection = 1.0 / windowHopCorrection;

	// --- set
	inputWriteIndex = 0;
	inputReadIndex = 0;

	outputWriteIndex = 0;
	outputReadIndex = 0;

	fftCounter = 0;

	// --- reset flags
	needInverseFFT = false;
	needOverlapAdd = false;

#ifdef HAVE_FFTW
	destroyFFTW();
	fft_input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * frameLength);
	fft_result = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * frameLength);
	ifft_result = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * frameLength);

	plan_forward = fftw_plan_dft_1d(frameLength, fft_input, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);
	plan_backward = fftw_plan_dft_1d(frameLength, fft_result, ifft_result, FFTW_BACKWARD, FFTW_ESTIMATE);
#endif
}

/**
\brief zero pad the input timeline

- NOTES:<br>

\param count the number of zero-valued samples to insert

\returns true if the zero-insertion triggered a FFT event, false otherwise
*/
bool PhaseVocoder::addZeroPad(unsigned int count)
{
	bool fftReady = false;
	for (unsigned int i = 0; i < count; i++)
	{
		// --- push into buffer
		inputBuffer[inputWriteIndex++] = 0.0;

		// --- wrap
		inputWriteIndex &= wrapMask;

		// --- check the FFT
		bool didFFT = advanceAndCheckFFT();

		// --- for a zero-padding operation, the last inserted zero
		//     should trigger the FFT; if not something has gone horribly wrong
		if (didFFT && i == count - 1)
			fftReady = true;
	}

	return fftReady;
}

/**
\brief advance the sample counter and check to see if we need to do the FFT.

- NOTES:<br>

\returns true if the advancement triggered a FFT event, false otherwise
*/
bool PhaseVocoder::advanceAndCheckFFT()
{
	// --- inc counter and check count
	fftCounter++;

	if (fftCounter != frameLength)
		return false;

	// --- we have a FFT ready
	// --- load up the input to the FFT
	for (int i = 0; i < frameLength; i++)
	{
		fft_input[i][0] = inputBuffer[inputReadIndex++] * windowBuffer[i];
		fft_input[i][1] = 0.0; // use this if your data is complex valued

		// --- wrap if index > bufferlength - 1
		inputReadIndex &= wrapMask;
	}

	// --- do the FFT
	fftw_execute(plan_forward);

	// --- in case user does not take IFFT, just to prevent zero output
	needInverseFFT = true;
	needOverlapAdd = true;

	// --- fft counter: small hop = more FFTs = less counting before fft
	//
	// --- overlap-add-only algorithms do not involve hop-size in FFT count
	if (overlapAddOnly)
		fftCounter = 0;
	else // normal counter advance
		fftCounter = frameLength - hopSize;

	// --- setup the read index for next time through the loop
	if (!overlapAddOnly)
		inputReadIndex += hopSize;

	// --- wrap if needed
	inputReadIndex &= wrapMask;

	return true;
}

/**
\brief process one input sample throug the vocoder to produce one output sample

- NOTES:<br>

\param input the input sample x(n)
\param fftReady a return flag indicating if the FFT has occurred and FFT data is ready to process

\returns the vocoder output sample y(n)
*/
double PhaseVocoder::processAudioSample(double input, bool& fftReady)
{
	// --- if user did not manually do fft and overlap, do them here
	//     this allows maximum flexibility in use of the object
	if (needInverseFFT)
		doInverseFFT();
	if(needOverlapAdd)
		doOverlapAdd();

	fftReady = false;

	// --- get the current output sample first
	double currentOutput = outputBuffer[outputReadIndex];

	// --- set the buffer to 0.0 in preparation for the next overlap/add process
	outputBuffer[outputReadIndex++] = 0.0;

	// --- wrap
	outputReadIndex &= wrapMaskOut;

	// --- push into buffer
	inputBuffer[inputWriteIndex++] = (double)input;

	// --- wrap
	inputWriteIndex &= wrapMask;

	// --- check the FFT
	fftReady = advanceAndCheckFFT();

	return currentOutput;
}

/**
\brief perform the inverse FFT on the processed data

- NOTES:<br>
This function is optional - if you need to sequence the output (synthesis) stage yourself <br>
then you can call this function at the appropriate time - see the PSMVocoder object for an example

*/
void PhaseVocoder::doInverseFFT()
{
	// do the IFFT
	fftw_execute(plan_backward);

	// --- output is now in ifft_result array
	needInverseFFT = false;
}

/**
\brief perform the overlap/add on the IFFT data

- NOTES:<br>
This function is optional - if you need to sequence the output (synthesis) stage yourself <br>
then you can call this function at the appropriate time - see the PSMVocoder object for an example

\param outputData an array of data to overlap/add: if this is NULL then the IFFT data is used
\param length the lenght of the array of data to overlap/add: if this is -1, the normal IFFT length is used
*/
void PhaseVocoder::doOverlapAdd(double* outputData, int length)
{
	// --- overlap/add with output buffer
	//     NOTE: this assumes input and output hop sizes are the same!
	outputWriteIndex = outputReadIndex;

	if (outputData)
	{
		for (int i = 0; i < length; i++)
		{
			// --- if you need to window the data, do so prior to this function call
			outputBuffer[outputWriteIndex++] += outputData[i];

			// --- wrap if index > bufferlength - 1
			outputWriteIndex &= wrapMaskOut;
		}
		needOverlapAdd = false;
		return;
	}

	for (int i = 0; i < frameLength; i++)
	{
		// --- accumulate
		outputBuffer[outputWriteIndex++] += windowHopCorrection * ifft_result[i][0];

		// --- wrap if index > bufferlength - 1
		outputWriteIndex &= wrapMaskOut;
	}

	// --- set a flag
	needOverlapAdd = false;
}

#endif

