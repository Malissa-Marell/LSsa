#include "input_functions.h"
#include "string.h"

void flush()
{
	while ((getchar()) != '\n')
		;
}

void read_string(const char *prompt, char *destination)
{
	printf("%s", prompt);
	scanf("%s", destination);
	flush();
}

float read_float(const char *prompt)
{
	float input;
	printf("%s", prompt);
	scanf("%f", &input);
	flush();
	return input;
}

int read_integer(const char *prompt)
{
	int input;
	printf("%s", prompt);
	scanf("%d", &input);
	flush();
	return input;
}

int read_integer_in_range(const char *prompt, int min, int max)
{
	int input;
	bool repeat = false;

	printf("%s", prompt);

	do
	{
		scanf("%d", &input);
		flush();

		if (input < min && input > max)
		{
			printf("Please enter a value between %d and %d: \n");
			repeat = true;
		}
		else
		{
			repeat = false;
		}
	} while (repeat);

	return input;
}

bool read_boolean(const char *prompt)
{
	char input[5];

	printf("%s", prompt);
	scanf("%s", input);

	if (strcmp(input, "y") == 0 ||
		strcmp(input, "Y") == 0 ||
		strcmp(input, "yes") == 0 ||
		strcmp(input, "Yes") == 0 ||
		strcmp(input, "YES") == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}
