/*
	gcode2eps - convert g-code to EPS
	
	Copyright 2004 Shawn Rutledge
	Licensed subject to the terms of the GNU Public License
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FALSE 0
#define TRUE 1

enum states { BEGIN, COMMENT, PARM, END, DONE };

int nearZero(float val)
{
	return fabs(val) < 0.0001;
}

float min(float one, float other)
{
	if (one < other)
		return one;
	else
		return other;
}

float max(float one, float other)
{
	if (one > other)
		return one;
	else
		return other;
}

float angle(float cx, float cy, float x, float y)
{
    float dx = x - cx;
	float dy = y - cy;
	float a;
	if (nearZero(dx))
	{
		if (dy < 0.0)
			return 270.0;
		else
			return 90.0;
	}
	else if (nearZero(dy))
	{
		if (dx < 0.0)
			return 180.0;
		else
			return 0.0;
	}
	else
	{
		a = atan(dy / dx) * 180 / M_PI;
		if (dx < 0)
			a += 180;
		return a;
	}
}

int main(int argc, char** argv)
{
	FILE* in;
	FILE* out = stdout;
	int i;
	char* line = NULL;
	size_t len = 0;
	ssize_t read;
	int pathOpen = FALSE;
	int pathExists = FALSE;
	float end_x, end_y, end_z, end_i, end_j;
	float min_x, min_y, min_z, max_x, max_y, max_z;

	// Check the arguments
	if (argc < 2 || argv[1][0] == '-')
	{
		fprintf(stderr, "Converts gcode for CNC machines to EPS.\n");
		// future...
		//fprintf(stderr, "Gray level is proportional to depth.\n");
		fprintf(stderr, "Usage: %s infile [outfile]\n", argv[0]);
		fprintf(stderr, "   if outfile is not given, STDOUT is assumed\n");
		exit(0);
	}
	in = fopen(argv[1], "r");
	if (in == NULL)
	{
		fprintf(stderr, "ERROR: cannot open %s for reading\n", argv[1]);
		exit(1);
	}
	
	if (argc > 2)
	{
		out = fopen(argv[2], "w");
		if (out == NULL)
		{
			fprintf(stderr, "ERROR: cannot open %s for writing\n", argv[2]);
			exit(1);
		}
	}
	
	// Get the extents, so we can output the BoundingBox comment.
	while ((read = getline(&line, &len, in)) != -1) 
	{
		enum states state = BEGIN;
		float temp;
		
		for (i = 0; i < read && state != COMMENT; ++i)
		{
			switch (state)
			{
			case BEGIN:
				switch (line[i])
				{
				case '{':
					state = COMMENT;
					break;
				case 'X':
					temp = atof(line + i + 1);
					min_x = min(min_x, temp);
					max_x = max(max_x, temp);
					break;
				case 'Y':
					temp = atof(line + i + 1);
					min_y = min(min_y, temp);
					max_y = max(max_y, temp);
					break;
				case 'Z':
					temp = atof(line + i + 1);
					min_z = min(min_z, temp);
					max_z = max(max_z, temp);
					break;
				}	
			}
		}
	}
	fclose(in);
	
	// Re-open the file.  Because of this it's probably not possible to
	// read from STDIN.
	in = fopen(argv[1], "r");
	if (in == NULL)
	{
		fprintf(stderr, "ERROR: cannot open %s for reading\n", argv[1]);
		exit(1);
	}
	
	// Output EPS header stuff
	fprintf(out, "%%!PS-Adobe-2.0 EPSF-2.0n\n");
	fprintf(out, "%%%%Title: %s\n", argv[1]);
	fprintf(out, "%%%%Creator: gcode2eps 0.1\n");
	fprintf(out, "%%%%BoundingBox: %d %d %d %d\n", 
		lrintf(min_x) - 1, lrintf(min_y) - 1, 
		lrintf(max_x) + 1, lrintf(max_y) + 1);
	
	fprintf(out, "gsave 1 setlinecap 1 setlinejoin\n");

	// Read G-codes and output PS commands
	while ((read = getline(&line, &len, in)) != -1) 
	{
		enum states state = BEGIN;
		int gcode = -1;
		float X, Y, Z, I, J, r, cx, cy, dx, dy, a1, a2;
		for (i = 0; i < read && state != COMMENT; ++i)
		{
			switch (state)
			{
			case BEGIN:
				switch (line[i])
				{
				case '{':
					state = COMMENT;
					break;
				case 'G':
					gcode = atoi(line + i + 1);
//printf("got gcode %d\n", gcode);			
					state = PARM;
					break;
				// if we find numbers, they aren't useful unti we have the gcode
				}
				break;
			case PARM:
				switch (line[i])
				{
				case 'X':
					X = atof(line + i + 1);
//printf("got X %f\n", X);			
					break;
				case 'Y':
					Y = atof(line + i + 1);
//printf("got Y %f\n", Y);			
					break;
				case 'Z':
					Z = atof(line + i + 1);
//printf("got Z %f\n", Z);
					break;
				case 'I':
					I = atof(line + i + 1);
//printf("got I %f\n", I);
					break;
				case 'J':
					J = atof(line + i + 1);
//printf("got J %f\n", J);
					break;
				case '\n':
				case '\r':
//printf("EOL\n");
					state = END;
					break;
				}
				break;
			}
			// Output PS path command for g-code
			if (state == END)
			{
				state = DONE;
				switch (gcode)
				{
				// G0: moveto
				case 0:
					if (pathOpen && pathExists)
					{
						fprintf(out, "stroke\n");
					}
					fprintf(out, "newpath %f %f moveto\n", X, Y);
					pathOpen = TRUE;
					pathExists = FALSE;
					break;
				// G1: lineto
				case 1:
					fprintf(out, "%f %f lineto\n", X, Y);
					pathExists = TRUE;
					break;
				// G2: arc cw
				case 2:
				    cx = end_x + I;
				    cy = end_y + J;
					r = sqrtf( I * I + J * J );
					//dx = X - cx;
					//dy = Y - dx;
					dx = end_x - cx;
					dy = end_y - cy;
					a1 = angle(cx, cy, end_x, end_y);
					a2 = angle(cx, cy, X, Y);
//printf("a1 %f, a2 %f\n", a1, a2);
					fprintf(out, "%f %f %f %f %f arcn\n", 
						cx, cy, r, a1, a2);
					pathExists = TRUE;
					break;
				// G3: arc ccw
				case 3:
				    cx = end_x + I;
				    cy = end_y + J;
					r = sqrtf( I * I + J * J );
					//dx = X - cx;
					//dy = Y - dx;
					dx = end_x - cx;
					dy = end_y - cy;
					a1 = angle(cx, cy, end_x, end_y);
					a2 = angle(cx, cy, X, Y);
//printf("a1 %f, a2 %f\n", a1, a2);
					fprintf(out, "%f %f %f %f %f arc\n", 
						cx, cy, r, a1, a2);
					pathExists = TRUE;
					break;
				}
				
				end_x = X;
				end_y = Y;
				end_z = Z;
				end_i = I;
				end_j = J;
			}
		}
		//printf("Retrieved line of length %zu :\n", read);
	}
	if (line)
		free(line);
	if (pathExists)
		fprintf(out, "stroke\n");
	fprintf(out, "grestore\n");
}
