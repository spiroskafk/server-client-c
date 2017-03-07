#include "MyHeader.h" /* for user-defined constants */
 
/** Functions Prototype's */
char randomZone();     									/* Tyxaia epilogh zwnhs */
char getZone();         								/* Epilegoume zoni (O user eisagei th zwnh pou thelei) */

void clean_stdin( void );                               /* Katharizei ton input buffer */
void sendZone( int socketfd, char zone );       		/* Stelnoume ston server tin zoni pou epilexthike */
void sendPositions( int socketfd, int positions );      /* Stelnoume ston server ton arithmo thesewn */
void readMessage( char, int, int, int );        		/* Diavazei to sendMessage() tou server */
void sendCard( int, int );                              /* Stelnoume tin pistotiki karta */
void printPositions( int[], int, int, int );   		    /* Tipwnoume tis theseis pou epilexthikan apo ton client */

int getPositions();     								/* Epilegoume arithmo thesewn (O user eisagei ton arithmo) */
int randomPosition();  									/* Tyxaia epilogh arithmous thesewn */
int randomCard();                                       /* Tuxaia epilogi pistotikis kartas */


/* Timer */  
time_t start_time;

     
/* Signal handler gia alarm. Minima signwmis pros ton client oso einai sthn anamonh */
void myalarm(int sign)
{
	printf( "Signwmi. Prepei na perimenete ligo akoma\n" );
	signal( SIGALRM, myalarm );
	alarm( 10 );
}
     
int main( void )
{
	/* Struct for the server socket address */
	struct sockaddr_un serverAddr;
           
	char zone;          /* I zwni pou epilegei o client( Eite apo random eite user input ) */
	
	int socketfd;       /* To file descriptor tou socket pou xrisimopoieitai gia tin epikoinwnia me ton server */
	int positions;      /* Oi theseis pou epilegei o client */
	int k = 0;          /* Xrisimopoieitai gia na ylopoihsoume to minima signwmis */
	int card;           /* I karta pou epilegete random */
     
	/* Anti gia srand(time(NULL)) gia megaliterh akriveia kai mh epanalipsi twn idiwn pseudotyxaiwn arithmwn */
	struct timeval tv;
	gettimeofday( &tv, NULL );
	unsigned long time_in_micros = 1000000000 * tv.tv_sec + tv.tv_usec;
	srand( time_in_micros );
           
	/* Enarksi timer */
	start_time = time( NULL );
	
	/* Dilwsei alarm */
	signal( SIGALRM, myalarm );
     
	/* Create the client's endpoint. */
	socketfd = socket( AF_LOCAL, SOCK_STREAM, 0 );
     
	/* Zero all fields of serverAddress */
	bzero( &serverAddr, sizeof( serverAddr ) );
           
	/* Xrisimopoioume af_local socket type epidi to connection ginete ston idio ypologisti */
	serverAddr.sun_family = AF_LOCAL;
           
	/* Orizoume to onoma tou socket */
	strcpy( serverAddr.sun_path, UNIXSTR_PATH );
     
	/* Connect the client's and the server's endpoint. */
	connect( socketfd, (struct sockaddr *) &serverAddr, sizeof( serverAddr ) );
           
           
	/* Alarm. Kathe 10 deutera tipwnetai to minima signwmis ston pelati 
	 * mexri na mpeis to critical section tou semaphorou tilefwniti. 
	 * Otan o client sundethei me ton tilefwniti stelnetai to write                 
	 * apo ton server kai diavazetai apo to read ston client. Epeita to alarm ginetai 0, opote to
	 * minima stamataei na stelnetai */
	alarm( 10 );
	read( socketfd, &k, sizeof( int ) );
	alarm( 0 );
           
	/* Eidopoihsh oti sundethike me ton server */	   
	printf( "\n======================================================\n" );
	printf( "Client(%d) you are connected\n", k );
           
	/* Get positions */
	//positions = getPositions();  /* User Input */
	positions = randomPosition();  /* Random input */
           
	/* Get zone */
	//zone = getZone();           /* User input */
	zone = randomZone();		  /* Random input */
	
	/* Get card */
	card = randomCard();
	
	/* Ektipwse Invalid message */
	if( card != 1 )
	{
		printf( "===================================================\n" );
		printf( "Invalid Card\n" );
		printf( "===================================================\n" );
	}
           
	/* Print */
	printf( "You are client(%d). You wanna reserve %d positions\n", k, positions );
	printf( "In %c 'ZONE' \n", zone );
	printf( "===================================================\n" );
           
	/* Steile tis theseis ston server */
	sendPositions( socketfd, positions );
           
	/* Steile tin zoni ston server */
	sendZone( socketfd, zone );
                       
	/* Steile tin karta ston server */
	sendCard( socketfd, card );
                       
	/* Read message from server */
	readMessage( zone, socketfd, positions, k );
	
	/* Close connection */
	close( socketfd );
}


/* Diavazei to eidos minimatos apo ton server
 * kai to ektipwnei ston client */     
void readMessage( char zone, int socketfd, int positions, int clientID )
{  
	int type;
	int arrayA[ 100 ];    /* Apothikeuete i A zoni */
	int arrayB[ 130 ];	  /* Apothikeuete i B zoni */
	int arrayC[ 180 ];	  /* Apothikeuete i C zoni */
	int arrayD[ 230 ];	  /* Apothikeuete i D zoni */
           
	/* Diavazete to eidos tou minimatos apo ton server
	 * kai oi pinakes - zones */
	read( socketfd, &type, sizeof( int ) );
	read( socketfd, &arrayA, sizeof( arrayA ) );
	read( socketfd, &arrayB, sizeof( arrayB ) );
	read( socketfd, &arrayC, sizeof( arrayC ) );
	read( socketfd, &arrayD, sizeof( arrayD ) );
	
    /* Analoga me to eidos tou minimatos ektipwnoume
	 * ta katallila stoixeia */
	if( type == 1 )
	{
		if( zone == 'A' )
		{
			printPositions( arrayA, 100, positions * 50, clientID );
		}
		else if( zone == 'B' )
		{
			printPositions( arrayB, 130, positions * 40, clientID );
		}
		else if( zone == 'C' )
		{
			printPositions( arrayC, 180, positions * 35, clientID );
		}
		else if( zone == 'D' )
		{
			printPositions( arrayD, 230, positions * 30, clientID );
		}
		return;
	}
	else if( type == 3 )
	{
		printf( "Den yparxoun theseis se aftin tin zwni.\n" );
	}
	else if( type == 4 )
	{
		printf( "Oles oi theseis tou theatrou gemisane.\n" );
	}

}
 
/* Ypologizei tuxaia tin karta */ 
int randomCard()
{
	int per;
       
	per = 1 + rand() % 10;
       
	if( per <= 9 )
	{
		return 1;
	}
	else
	{
		return -1;
	}
}
 
/* Ypologizei tuxaia theseis */
int randomPosition()
{
	int per;
           
	per = 1 + rand() % 4;
	return per;
}
     
/* Ypologizei mia tuxaia zwni */	 
char randomZone()
{
	int per;
		
	per = 1 + rand() % 100;
	
	if( per <= 10 )
	{
		return 'A';
	}
	else if( per <= 30 )
	{
		return 'B';
	}
	else if( per <= 60 )
	{
		return 'C';
	}
	else
	{
		return 'D';
	}
}
     
/* Ektipwnei tis theseis pou ekleise o kathe client */	 
void printPositions( int array[], int length, int amount, int clientID )
{
	int i = 0;
	int count = 0;
		
	printf( "===================================================\n" );
	printf( "O Client(%d) ekleise tis ekseis theseis : \n", clientID );
                       
	for( i = 0; i < length; i++ )
	{
		if( array[i ] == clientID )
		{
			if( length == 100 )
			{
				printf( "A%d ", i );
			}
			else if( length == 130 )
			{
				printf( "B%d ", i );
			}
			else if( length == 180 )
			{
				printf( "C%d ", i );
			}
			else if( length == 230 )
			{
				printf( "D%d ", i );
			}
		}
	}
               
	printf( "\nTo kostos einai : %d\n", amount );
	printf( "===================================================\n" );
}


/* Diavazei tis theseis apo ton xristi */     
int getPositions()
{
	int user_input = 0;
	int valid_input = 0;
     
	while( valid_input == 0 )
	{
		printf( "How many positions you wanna reserve?(1 - 4) : " );
		scanf( "%d", &user_input );
		
		if ( user_input > 0 && user_input <= 50 )
		{
			valid_input = 1;
		}
		else
		{
			printf( "\007Error: Invalide choice\n" );
		}
	}
           
	// clean input buffer
	clean_stdin();
           
	return user_input;
}
    
/* Stelnei tis theseis ston client */	
void sendPositions( int socketfd, int positions )
{
	write( socketfd, &positions, sizeof( int ) );
}
     
/* Stelnei tin karta ston client */	 
void sendCard( int socketfd, int card )
{
	write( socketfd, &card, sizeof( int ) );
}
     
/* Diavazei tin zwni apo ton xristi */	 
char getZone()
{
	char user_input = 0;
	char valid_input = 0;
		
	while( valid_input == 0 )
	{
		printf( "In which zone you want to reserve positions?(A-D) : " );
		scanf( " %c", &user_input );
		user_input = toupper( user_input );
                   
		if( ( user_input == 'A' ) || ( user_input == 'B' ) ||
		( user_input == 'C' ) || ( user_input == 'D' ) )
		{
			valid_input = 1;
		}
		else
		{
			printf( "\007Error : Invalid Choice\n" );
		}
	}
           
	// clean input buffer
	clean_stdin();
           
	return user_input;
}
     
/* Stelnei tin zwni ston server */	 
void sendZone( int socketfd, char zone )
{
	write( socketfd, &zone, sizeof( char ) );
}
     
/* Katharizei ton stdin buffer */	 
void clean_stdin( void )
{
	int c;
           
	do 
	{
		c = getchar();
	} while( c!= '\n' && c != EOF );
}