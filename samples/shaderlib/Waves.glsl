/*
	Wave functions
*/

// Wave
float SineWave (float samp, float freq)	// --U`U--
{
	return Sin( Pi() * samp * freq / SAMPLE_RATE );
}

float SawWave (float samp, float freq)	//  --/|/|--
{
	return Fract( samp * freq / SAMPLE_RATE ) * 2.0 - 1.0;
}

float TriangleWave (float samp, float freq)	// --/\/\--
{
	float value = Fract( samp * freq / SAMPLE_RATE );
	return Min( value, 1.0 - value ) * 4.0 - 1.0;
}

// Gain
float LinearGain (float samp, float value, float startTime, float endTime)
{
	float start = startTime * SAMPLE_RATE;
	float end   = endTime * SAMPLE_RATE;
	return samp > start and samp < end ?
			value * (1.0 - (samp - start) / (end - start)) :
			0.0;
}

float ExpGain (float samp, float value, float startTime, float endTime)
{
	float start = startTime * SAMPLE_RATE;
	float end   = endTime * SAMPLE_RATE;
	float power = 4.0;
	return samp > start and samp < end ?
			value * (1.0 - pow( power, (samp - start) / (end - start) ) / power) :
			0.0;
}
