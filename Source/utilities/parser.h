#pragma once
#include "PL/pl_math.h"

static inline b32 is_whitespace(char ch)
{
	return (ch == ' ' || ch == '\t' || ch == '\r');
}

static inline b32 is_digit(char nm)
{
	return (nm >= '0' && nm <= '9');
}

static inline b32 is_exponent(char ex)
{
	return (ex == 'e' || ex == 'E');
}

static inline char* skip_to_new_line(char* ptr)
{
	while (*ptr != '\n')
	{
		ptr++;
	}
	return ptr;
}

static inline char* skip_whitespace(char* ptr)
{
	while (is_whitespace(*ptr))
	{
		ptr++;
	}
	return ptr;
}

//Parses a number steam into a int32 
static inline char* parse_int(char* from, int32& val)
{
	from = skip_whitespace(from);
	int32 sign;
	switch (*from)
	{
	case '+':
		sign = 1;
		from++;
		break;
	case '-':
		sign = -1;
		from++;
		break;
	default:
		sign = 1;
		break;
	}

	val = 0;
	while (is_digit(*from))
	{
		val = (val * 10) + (*from - '0');
		from++;
	}
	val = val * sign;
	return from;
}

//Parses a number steam into a int64
static inline char* parse_int(char* from, int64& val)
{
	from = skip_whitespace(from);
	int64 sign;
	switch (*from)
	{
	case '+':
		sign = 1;
		from++;
		break;
	case '-':
		sign = -1;
		from++;
		break;
	default:
		sign = 1;
		break;
	}

	val = 0;
	while (is_digit(*from))
	{
		val = (val * 10) + (*from - '0');
		from++;
	}
	val = val * sign;
	return from;
}

//Parses a number steam into a uint64 (ignores sign)
static inline char* parse_uint(char* from, uint64& val)
{
	from = skip_whitespace(from);

	val = 0;
	while (is_digit(*from))
	{
		val = (val * 10) + (*from - '0');
		from++;
	}
	return from;
}

//ASSESS: Whether this method of parsing fractional part and whole number part into uint64s, then casting them into a f64 loses more precision than normal.
//parses a float into val, reading from *from onwards and returns buffer position after parsing
static inline char* parse_f64(char* from, f64& val)
{
	f64 sign;
	int32 frac_prec, exponent, exponent_sign;
	uint64 fraction, front_num;
	from = skip_whitespace(from);
	
	switch (*from)
	{
	case '+':
		sign = 1.0;
		from++;
		break;
	case '-':
		sign = -1.0;
		from++;
		break;
	default: 
		sign = 1.0;
		break;
	}

	front_num = 0;
	while (is_digit(*from))
	{
		front_num = (front_num * 10) + (*from - '0');
		from++;
	}

	if (*from == '.')
	{
		from++;
	}

	fraction = 0;
	frac_prec = 0;
	while (is_digit(*from))
	{
		frac_prec++;
		fraction = (fraction * 10) + (*from - '0');
		from++;
	}

	front_num *= INT_POWER_10[frac_prec];
	front_num += fraction;						//combining the whole number and the decimal part. Will shift later along with exponent after f64 conversion. 

	exponent = 0;
	if (is_exponent(*from))
	{
		from++;
		switch (*from)
		{
		case '+':
			exponent_sign = 1;
			from++;
			break;
		case '-':
			exponent_sign = -1;
			from++;
			break;
		default:
			exponent_sign = 1;
			break;
		}
		while (is_digit(*from))
		{
			exponent = 10 * exponent + (*from - '0');
			from++;
		}
		exponent = exponent * exponent_sign;
	}
	exponent -= frac_prec;									//Adding back the decimal shift that was ignored

	val = (f64)front_num;
	val = val * sign;
	exponent = (exponent >= -AT_POWER_10_OFFSET && exponent <= 19) ? exponent : 0;
	val *= F64_POWER_10[exponent + AT_POWER_10_OFFSET];
	return from;
}

//parses a vec3f and returns pointer after parsing
inline char* parse_vec3f(char* pos, vec3f& vec)
{
	char* ptr;
	f64 flt;
	ptr = parse_f64(pos, flt);
	vec.x = (f32)flt;
	ptr = parse_f64(ptr, flt);
	vec.y = (f32)flt;
	ptr = parse_f64(ptr, flt);
	vec.z = (f32)flt;
	return ptr;
}


#undef AT_MAX_POWER_OFFSET