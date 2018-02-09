/*********************************************/
/*       Counter Manangement Functions       */
/*********************************************/

#define MAX_NO_OF_COUNTER_ENTRIES MAX_NO_OF_CONNECTIONS
int searchCounterFile(FILE *f, int serial)
{
int s,c,r=-1;
rewind(f);
while (fscanf(f,"%9d %9d\n",&s,&c) != EOF)
	if (s == serial)
		{
		r=c;
		break;
		}
return r;
}

void updateCounterFileEntry(FILE *f, int serial, int counter)
{
int i,n=0,s,c,found=0;
int serialList[MAX_NO_OF_COUNTER_ENTRIES];
int counterList[MAX_NO_OF_COUNTER_ENTRIES];
rewind(f);
while ((fscanf(f,"%9d %9d\n",&s,&c) != EOF) && (n<MAX_NO_OF_COUNTER_ENTRIES))
	{
	serialList[n]=s;
	if (s != serial)
		counterList[n]=c;
	else
	{
		counterList[n]=counter;
		found=1;
	}
	n++;
	}
rewind(f);
ftruncate(fileno(f),0);
for (i=0;i<n;i++)
	fprintf(f,"%d %d\n",serialList[i],counterList[i]);
if (!found)
{
	printf("Writing NEW serial/counter\n");
	fprintf(f,"%d %d\n",serial,counter);
}
	fflush(f);
return;
}

