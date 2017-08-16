#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"

/*
* Short scale units
* http://en.wikipedia.org/wiki/Short_scale
*/
static const char* short_scale[] = {
	"",
	"k",//"thousand",
	"m",//"million",
	"b",//"billion",
	"t",//"trillion",
	"quadrillion",
	"quintillion",
	"sextillion",
	"septillion"
};

/*
* Long scale units
* http://en.wikipedia.org/wiki/Short_scale
*/
static const char* long_scale[] = {
	"",
	"thousand",
	"million",
	"milliard",
	"billion",
	"billiard",
	"trillion",
	"trilliard",
	"quadrillion"
};

/*
* Convert number to human readable string using
* the given naming system.
*/
const char* scale(double n, int decimals, const char* units[])
{
	/*
	* Number of digits in n is given by
	* 10^x = n ==> x = log(n)/log(10) = log_10(n).
	*
	* So 1000 would be 1 + floor(log_10(10^3)) = 4 digits.
	*/
	int digits = n == 0 ? 0 : 1 + (int)floor((double)log10l(fabs(n)));

	// determine base 10 exponential
	int exp = digits <= 4 ? 0 : 3 * ((digits - 1) / 3);

	// normalized number
	double m = n / (double)powl(10, exp);

	// no decimals? then don't print any
	if (m - (long)(n) == 0)
		decimals = 0;

	// don't print unit for exp<3
	static char s[32];
	static const char* fmt[] = { "%1.*lf%s", "%1.*lf" };
	sprintf(s, fmt[exp<3], decimals, m, units[exp / 3]);
	return s;
}

/*
* Convert number to short scale representation
*/
const char* sscale(double n, int decimals)
{
	static char s[32];
	strcpy(s, scale(n, decimals, short_scale));
	return s;
}

/*
* Convert number to long scale representation
*/
const char* lscale(double n, int decimals)
{
	static char s[32];
	strcpy(s, scale(n, decimals, long_scale));
	return s;
}

char *sprintf_num(double n, char *outstr, int outstrlen)
{
	if (!outstr)
		return 0;
	const char *s = sscale(n, 2);
	size_t len = strlen(s);
	memset(outstr, 0, outstrlen);
	return strncpy(outstr, s, min(len, outstrlen));
}


/*
	by Angel Rapallo
	format an integer with up to 20 digits length
	with commas separators for hundreds, thoushands, etc.
	this function only uses chars not strings and does not use
	any locale or external library except the _i64toa_s api
	to convert the integer into an array of chars.

	it is a flat algorithm, is does not use recursion or any
	other method.

	at its worst case it can iterate around 24 times only.
	for example if the number has 20 digits(MAXIMUM it can handle)
	then it will iterate 20 / 4 = 5 iterations + 20 to copy the digits
	a total os 24 iterations.

	the algorithm is
	convert number to array[]
	get number of digits aka the length of the array
	get the position of the first comma
	get the total length of the formatted array digits + # of commas.
	create dynamic array with this information to hold the formatted number.
	put the commas in place for example
	if number is 11111 then
	[][, ][][][][]
	the put the digits in place
	[1][1][, ][1][1][1]

	as allways there must be a shorter and better way, but i havent found it so
	i came up with this one.one way is to use two stacks and push the digits
	into it then pop all the digits and include the commans using the % operator
	to check if a comma belongs after the digits.i like this one because it does not
	use anything except chars.

	the idea of placing commas into a number seems simple enough
	just go one character at a time and check if a comma belongs there
	the problem is that to do that you would need a way to insert chars
	into an array, which is what this algorith does but into steps instead
	of one.

	I hope it can help some one.
*/
char* sprintf_num_commas(long long n, char *outstr, int outstrlen)
{
	if (!outstr || outstrlen<32)
		return 0;

	memset(outstr, 0, outstrlen);

	// convert number to an array of chars
	char _number_array[32] = { '\0' };
	_i64toa_s(n, _number_array, sizeof(_number_array), 10);
	char* _number_pointer = _number_array;
	int _number_of_digits = 0;
	while (*(_number_pointer + _number_of_digits++));
		--_number_of_digits;

	// count the number of digits
	// calculate the position for the first comma separator
	// calculate the final length of the number with commas
	// the starting position is a repeating sequence 123123... which depends on the number of digits
	// the length of the number with commas is the sequence 111222333444...
	int _starting_separator_position = _number_of_digits < 4 ? 0 : _number_of_digits % 3 == 0 ? 3 : _number_of_digits % 3;
	int _formatted_number_length = _number_of_digits + _number_of_digits / 3 - (_number_of_digits % 3 == 0 ? 1 : 0);

	// create formatted number array based on calculated information.
	char* _formatted_number = outstr;

	// place all the commas
	for (int i = _starting_separator_position; i < _formatted_number_length - 3; i += 4)
		_formatted_number[i] = ',';

	// place the digits
	for (int i = 0, j = 0; i < _formatted_number_length; i++)
		if (_formatted_number[i] != ',')
			_formatted_number[i] = _number_pointer[j++];

	// close the string
	_formatted_number[_formatted_number_length] = '\0';

	return _formatted_number;
}