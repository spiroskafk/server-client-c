#include "MyHeader.h" /* Yparxoun ola ta include edw */
   
                       
/* Size of the request queue. */
#define LISTENQ 20
         
 
/* STRUCT DEFINITION */
typedef struct
{
	float sunXronosAnamonis;   /* Sunolikos xronos anamonis */
	float sunXronosEksip;      /* Sunolikos xronos eksipiretisis */
	
	int zoneA[ 100 ];          /* Antiproswpeuei tin A zwni */
	int zoneB[ 130 ];
	int zoneC[ 180 ];
	int zoneD[ 230 ];
	int accountTransfer[ 100 ];/* Pinakas metaforas logariasmou etairas sto logariasmo theatrou */
	
	int companyAccount;        /* Logariasmos etairias kratisewn */
	int failOrders; 		   /* Apotiximenes paraggelies */
	int cardValid;			   /* Egkurotita kartas */
	int cardInvalidTimes;	   /* Fores pou i pistwtiki den itan egkuri */
}theatre;
     
 
/* Functions prototype's */
double calcPercentage( int totalOrders, int failOrders );       /* Pososto apotiximenwn/epituximenwn prospatheiwn */
 
int readPositions( int );       								/* Diavazei ton arithmo thesewn pou epilexthikan apo ton pelati*/
int isFull( theatre *ptr );     								/* Elegxoume an exei gemisei to theatro*/
int seatCheck( char zone, int positions, theatre *ptr );        /* Elegxei tis theseis tou pinaka
																   thesewn ths zwnhs pou epilexthike */																   
int searchInA( int, theatre * );      							/* Elegxei an yparxoun adeies theseis ston pinaka zwnhs A */
int searchInB( int, theatre * );        
int searchInC( int, theatre * );       
int searchInD( int, theatre * );        
int chargeSystem( char, int );   								/* Ypologismos xrewshs gia kathe pelati */

char readZone( int);           									/* Diavazei th zwnh pou epilexthike apo ton pelati */
 
void initializeStruct( theatre * );    							/* Arxikopoioume tous pinakes thesewn. Enas pinakas gia kathe zwnh */

void reservePositions( char zone, int positions, theatre *ptr );/* Ekxwroume tis kratiseis se adeies theseis twn pinakwnthesewn */
 
void sendMessage( int type, int connfd, theatre *ptr );         /* Stelnei ston client mia metavliti type kai tis zwnes
																 * etsi wste o client na ektipwsei to katallilo minima */
 
void printZones( theatre *ptr );        						/* Ektupwsh pinaka thesewn */

      
         
// Global Variables 
int CLIENTID = 0;        /* Arithmos pelati */
int theatreAccount = 0;  /* Logariasmos theatrou */
int totalOrders = 0;     /* Sunolikos arithmos paraggeliwn */
int count = 0;           /* Metritis gia ton pinaka metaforwn xrimatwn */
theatre *str;            /* Deiktis pros tin struct */

time_t start_time;       /* Timer */
         
       
/* The use of this function avoids the generation of "zombie" processes. */
void sig_chld( int signo )
{
	pid_t pid;
	int stat;
         
	while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 )
	{
		printf( "Child %d terminated.\n", pid );
	}
}

/* sinartisi gia to SIGINT */
void sig_server( int sig )
{
	int i;
	
	printf( "\n\nSTALTHIKE SIGNAL NA EKTIPWTHOUN TA PARAKATW\n\n" );
	printZones( str );
	
	printf( "\n\n==========================================================\n" );
	printf( "Pososto apotiximenwn : %lf\n", calcPercentage( totalOrders, str->failOrders ) );
	printf( "Total orders = %d\n", totalOrders );
	printf( "Fail orders = %d\n" , str->failOrders );
                                           
	printf( "Mesos xronos anamonis twn client einai %f\n", ( str->sunXronosAnamonis / CLIENTID ) );
	printf( "Mesos xronos eksipirtetis twn client einai %f\n", ( str->sunXronosEksip / CLIENTID ) );
	printf( "I karta itane akuri = %d\n", str->cardInvalidTimes );
	printf( "Logariasmos tou theatrou = %d\n", theatreAccount );		
	printf( "==========================================================\n" );
	
	printf( "Pinakas metaforwn xrimatwn\n" );
	for( i = 0; i < 100; i++ )
	{
		if( i % 10 == 0 )
		{
			printf( "\n" );
		}
		printf( "%d ", str->accountTransfer[ i ] );
	}
	
	kill( 0, SIGKILL );
}
         

/* Signal handler for alarm function */
/* Edw ginetai h metafora tou logariasmou ths etaireias ston logariasmo theatrou */
void myalarm(int sign)
{
	printf("Alarm! got signal at time %d\n", time( NULL ) - start_time );
	printf("Transfer...\n");
	
	str->accountTransfer[ count ] = str->companyAccount;
	theatreAccount += str->companyAccount;
	str->companyAccount = 0;
	count++;
	
	printf( "Theatre account is %d\n", theatreAccount );
	signal( SIGALRM, myalarm );
	alarm( 30 );
}
         
         
/* Start of main program */
int main( int argc, char **argv )
{
	struct sockaddr_un cliaddr, servaddr;
               
	// timers
	time_t time_an;
	time_t time_ek;
               
	float xronosAnamonis; /* Xronos anamonis ana pelati */
	float xronosEksip;    /* Xronos eksipiretisis ana pelati */
	char zone;            /* I zwni pou epelekse o client */
	int charge;           /* Xrewsi */         
	int listenfd;         
	int connfd;
	int shmid;            /* To id tis shared memory */
	int positions;        /* O arithmos twn thesewn pou epelekse o client */
	int seatFound = 0;    /* Apothikeuoume an vrikame thesi */
	int k;
	int card;             /* I timi tis kartas pou diavastike apo ton client */
	int i;                /* Counter */
	
	pid_t childpid;       /* To pid tis fork */
               
	//Semaphores
	sem_t *semTil;  /* Semaphoros tilefwniti */
	sem_t *semMem;  /* Semaphoros koinhs mnimis */
	sem_t *semTrap; /* Semaphoros trapezitwn */
	sem_t *semSem;  /* Semaphoros anazitisis */
 
	key_t shmkey;   /* To kleidi tis shared memory */
	
	socklen_t clilen;
               
	/* Shared Memory code - Create shared memory */
	shmid = shmget( 1025, 100, 0644 | IPC_CREAT );
	if (shmid < 0)
	{
		perror ("shmget\n");
		exit( 1 );
	}
               
	/* Attach struct to shared memory */
	str = ( theatre * ) shmat ( shmid, NULL, 0 );
               
	/* Dimiourgia semaphore */
	semTil = sem_open ( "semTil", O_CREAT | O_EXCL, 0644, 10 );
	sem_unlink( "semTil" );
	
	semMem = sem_open ( "semMem", O_CREAT | O_EXCL, 0644, 1 );
	sem_unlink( "semMem" );
	
	semTrap = sem_open ( "semTrap", O_CREAT | O_EXCL, 0644, 4 );
	sem_unlink( "semTrap" );
	
	semSem = sem_open ( "semSem", O_CREAT | O_EXCL, 0644, 1 );
	sem_unlink( "semSem" );
               
	/* Arxikopoihsh stoixeiwn tis struct */
	initializeStruct( str );
               
	/* Avoid "zombie" process generation. */
	signal( SIGCHLD, sig_chld );
	
	/* Signal pou termatizei ton server kai emfanizei apotelesmata */
	signal(SIGINT, sig_server);
 
	/* Ksekinaei na metraei o timer start_time */
	start_time = time( NULL );
               
	/* Dilwsh signal handler gia thn alarm */
	signal( SIGALRM, myalarm );
         
	/* Create the server's endpoint */
	listenfd = socket( AF_LOCAL, SOCK_STREAM, 0 );
         
	/* Remove any previous socket with the same filename */
	unlink( UNIXSTR_PATH );
               
	/* Zero all fields of servaddr. */
	bzero( &servaddr, sizeof( servaddr ) );
               
	/* Socket type is local (Unix Domain). */
	servaddr.sun_family = AF_LOCAL;
               
	/* Define the name of this socket. */
	strcpy( servaddr.sun_path, UNIXSTR_PATH );
         
	/* Create the file for the socket and register it as a socket. */
	bind( listenfd, ( struct sockaddr* ) &servaddr, sizeof( servaddr ) );
               
	/* Create request queue. */
	listen( listenfd, LISTENQ );
         
	/* Ksekinaei to alarm */
	alarm( 30 );
 
	for( ;; )
	{
		clilen = sizeof( cliaddr );
         
		/* Copy next request from the queue to connfd and remove it from the queue. */
		connfd = accept( listenfd, ( struct sockaddr * ) &cliaddr, &clilen );
         
		if ( connfd < 0 )
		{
			if ( errno == EINTR )
			{
				/* Something interrupted us */
				continue;
			}      
			else
			{
				fprintf( stderr, "Accept Error\n" );
				exit( 0 );
			}
		}
         	
		/* Spawn a child */
		childpid = fork();
		
		/* Auksanoume to id tou client */
		CLIENTID++;
                       
		/* Neos client dimiourgithike opote auksanoume tis paraggelies */
		totalOrders++;
                                       
		if ( childpid == 0 )
		{			
			/* Close listening socket. */
			close( listenfd ); 
                               
			/* Attach shared memory sto child */
			if ( (shmid = shmget( 1025, 100, 0644 ) ) < 0 )
			{
				printf( "error shmget inside child\n" );
				exit( 1 );
			}
			str = (theatre *)shmat( shmid, NULL, 0 );
                               
			/* Ksekinaei na metraei o timer anamonis */
			time_an = time( NULL );
                               
			/* Semaphore tilefwniti */
			sem_wait( semTil );
                               
			/* Sorry to client (Stelnetai se ena read tou client wste to alarm na ginei 0 
			 * otan o client sundethei me ton tilefwniti. Mexri tote to alarm einai 10 kai 
			 * ektipwnei to minima signwmis kathe 10 deutera. */
			write(connfd, &CLIENTID, sizeof( int ) );
                               
			/* Welcome message */
			printf( "Welcome %d client\n", CLIENTID );
 
			/* Ypologismos xronou anamonis ana client */
			xronosAnamonis = time( NULL ) - time_an;
							
			/* Prostithete ston sunoliko xrono anamonis */
			str->sunXronosAnamonis += xronosAnamonis;
 
			/* Ksekinaei na metraei o timer eksipiretisis */
			time_ek = time( NULL );
                               
			/* Read positions apo ton client */
			positions = readPositions( connfd );
                               
			/* Read zone apo ton client */
			zone = readZone( connfd );
			                                                               
			/* Read card from client */
			card = readCard( connfd ); 
			
			/* Edw Ginete o elenxos tis kartas - Sundesi me tin trapeza */
            if( fork() == 0 )
			{
				/* Semaphore trapezas */
				sem_wait ( semTrap);
													
				/* Kathisterisi 2 deuteroleptwn mexri na ginei o elegxos */
				sleep( 2 );
                                           
				/* An i karta einai 1 simainei oti einai egkuri
				 * opote apothikeuoume sto cardValid p exoume stin struct mas
				 * to 1 */
				if( card == 1 )
				{
					str->cardValid = 1;
				}
				else
				{
					/* An den einai 1 auksanoume tis apotiximenes paraggelies */
					str->cardValid = -1;
					str->failOrders++;	
					str->cardInvalidTimes++;
				}
                         
				/* End semaphore trapezas */		 
				sem_post( semTrap );
				exit( 0 );						
			}
			else
			{
				// Do nothing here
			}
                               
			/* Semaphore anazitisis */
			sem_wait( semSem );
                               
			/* Elegxos gia eleftheres theseis stin sigkekrimeni zwni
			 * p epelekse o client */
			seatFound = seatCheck( zone, positions, str );
			
			/* An den vroume theseis auksanoume tis apotiximenes paraggelies 
			 * kai stelnoume minima ston client */
			if( seatFound == 0 )
			{
				str->failOrders++;
				sendMessage( 3, connfd, str );													
			}
                                     
			/* Kathisterisi 6 sec mexri n ginei i anazitisi */              
			sleep( 6 );
			
            /* Telos semaphore anazitisis */           
			sem_post( semSem );
			
			/* An vrike eleftheres theseis kai i pistwtiki
			 * einai egkuri proxarame se kratisi thesewn */
			if( str->cardValid == 1 && seatFound == 1 )
			{
				/* Shared Memory semaphore */
				sem_wait( semMem );
 
				/* Katoxirwsi thesewn */
				reservePositions( zone, positions, str );
				
				/* End of shared memory semaphore */
				sem_post( semMem );
         
				/* Sistima xrewsis */
				charge = chargeSystem( zone, positions );
                                       
				/* Update company account */
				str->companyAccount += charge;
                                       
				/* Stelnoume minima epitixias ston client */
				sendMessage( 1, connfd, str );
			}

			/* Ypologismos xronou eksipiretisis */
			xronosEksip = time( NULL ) - time_an;
 
			/* Prostithetai ston sunoliko xrono eksipiretisis */
			str->sunXronosEksip += xronosEksip;                  
         
			/* Telos semaphore tilefwniti */
			sem_post( semTil );
			
			/*if( str->cardValid != 1 )
			{
				sendMessage( 5, connfd, str );
			}*/
              
			/* Detach Shared Memory */
			shmdt( str );
			
			/* Kleinoume tous semaphores */
			sem_close( semTil );
			sem_close( semMem );
			sem_close( semTrap );
			sem_close( semSem );
			
			exit( 0 );
		}
                       
		/* Elegxoume an gemisan oloi oi theseis tou pinaka. An nai 
		 * tote ektipwnoume ta telika apotelesmata */
		if( isFull( str ) == 1 )
		{
			printf( "\n\nSTALTHIKE SIGNAL NA EKTIPWTHOUN TA PARAKATW\n\n" );
			printZones( str );
			
			printf( "\n\n==========================================================\n" );
			printf( "Pososto apotiximenwn : %lf\n", calcPercentage( totalOrders, str->failOrders ) );
			printf( "Total orders = %d\n", totalOrders );
			printf( "Fail orders = %d\n" , str->failOrders );
                                           
			printf( "Mesos xronos anamonis twn client einai %f\n", ( str->sunXronosAnamonis / CLIENTID ) );
			printf( "Mesos xronos eksipirtetis twn client einai %f\n", ( str->sunXronosEksip / CLIENTID ) );
			printf( "I karta itane akuri = %d\n", str->cardInvalidTimes );
			printf( "Logariasmos tou theatrou = %d\n", theatreAccount );		
			printf( "==========================================================\n" );
	
			printf( "Pinakas metaforwn xrimatwn\n" );
			for( i = 0; i < 100; i++ )
			{
				if( i % 10 == 0 )
				{
					printf( "\n" );
				}
				printf( "%d ", str->accountTransfer[ i ] );
			}
			/* Stelnoume to minima oti oles oi theseis exoun gemisei */
			sendMessage( 4, connfd, str );
			exit( 0 );
		}
		
		/* Close connection */
		close( connfd );
	}
 
	/* Diagrafi koinis mnimis */
	shmctl( shmid, IPC_RMID, NULL );
}

         
/* Diavazei apo ton client th pistwtikh karta */
int readCard( int connfd )
{
	int card;
               
	read( connfd, &card, sizeof( int ) );
	return card;
}
     
/* Elenxoume an exoun gemisei oi zwnes - pinakes */	 
int isFull( theatre *ptr )
{
	int i;
	int count = 0;
               
	for( i = 0; i < 100; i++ )
	{
		if( ptr->zoneA[ i ] != 0 )
		{
			count++;
		}
	}
               
	for( i = 0; i < 130; i++ )
	{
		if( ptr->zoneB[ i ] != 0 )
		{
			count++;
		}
	}
               
	for( i = 0; i < 180; i++ )
	{
		if( ptr->zoneC[ i ] != 0 )
		{
			count++;
		}
	}      
               
	for( i = 0; i < 230; i++ )
	{
		if( ptr->zoneD[ i ] != 0 )
		{
			count++;
		}
	}
            
	/* Otan to count ginei 640 simanei oti oles oi theseis
	 * exoun katoxirwthei */
	if( count == 640 )
	{
		return 1;
	}
               
	return 0;
                               
}
 
/* Ypologizoume to pososto apotiximenwn paraggeliwn */ 
double calcPercentage( int totalOrders, int failOrders )
{
	double d;
               
	d = ( ( double ) failOrders * 100 ) / ( double )totalOrders;
               
	return d;
}

/* Ektipwnoume tous pinakes me ta apotelesmata */
void printZones( theatre *ptr )
{
	int i;
               
	// Print Zone A
	for( i = 0; i < 100; i++ )
	{
		printf( "A%d, ", ptr->zoneA[ i ] );
	}
     
	printf( "\n\n" );
     
	// Print Zone b 
	for( i = 0; i < 130; i++ )
	{
		printf( "B%d, ", ptr->zoneB[ i ] );
	}
     
	printf( "\n\n" );
      
	// Print Zone C
	for( i = 0; i < 180; i++ )
	{
		printf( "C%d, ", ptr->zoneC[ i ] );
	}
     
	printf( "\n\n" );
     
	// Print Zone D
	for( i = 0; i < 230; i++ )
	{
		printf( "D%d, ", ptr->zoneD[ i ] );
	}
               
}
         
void sendMessage( int type, int connfd, theatre *ptr )
{
	// TYPE = 1 : Minima epitixias kratisis
	// TYPE = 3 : Minima oti den yparxoun theseis se afti ti zwni
	// TYPE = 4 : Minima oti to theatro gemise
	
	write( connfd, &type, sizeof( int ) );
	write( connfd, &ptr->zoneA, sizeof( ptr->zoneA ) );
	write( connfd, &ptr->zoneB, sizeof( ptr->zoneB ) );
	write( connfd, &ptr->zoneC, sizeof( ptr->zoneC ) );
	write( connfd, &ptr->zoneD, sizeof( ptr->zoneD ) );
}

/* Ypologizoume tin xrewsi tou kathe client analoga
 * me tin zwni pou exei epileksei */
int chargeSystem( char zone, int positions )
{
	int amount;
		
	if( zone == 'A' )
	{
		amount = positions * 50;
	}
	else if( zone == 'B' )
	{
		amount = positions * 40;
	}
	else if( zone == 'C' )
	{
		amount = positions * 35;
	}
	else if( zone == 'D' )
	{
		amount = positions * 30;
	}
               
	return amount;
}

/* Edw ginete i kratisi twn thesewn. I logiki einai i eksis
 * Psaxnoume ton pinaka mexri na vroume tin prwti eleftheri thesi
 * Epeita kanoume mia defteri epanalipsi me metriti ton arithmo twn thesewn p exei dwsei o client
 * kai apothikeoume stin katallili zwni to CLIENTID */
void reservePositions( char zone, int positions, theatre *ptr )
{
	int i;
	int j;
     
	// Elegxos stin zoni A 
	if( zone == 'A' )
	{
		for( i = 0; i < 100; i++ )
		{
			if( ptr->zoneA[ i ] == 0 )
			{
				j = 0;
				while( j < positions )
				{
					ptr->zoneA[ i + j ] = CLIENTID;
					j++;
				}
				break;
			}
		}
	}
	else if( zone == 'B' )
	{
		for( i = 0; i < 130; i++ )
		{
			if( ptr->zoneB[ i ] == 0 )
			{
				j = 0;
				while( j < positions )
				{
					ptr->zoneB[ i + j ] = CLIENTID;
					j++;
				}
				break;
			}
		}
	}
	else if( zone == 'C' )
	{
		for( i = 0; i < 180; i++ )
		{
			if( ptr->zoneC[ i ] == 0 )
			{
				j = 0;
				while( j < positions )
				{
					ptr->zoneC[ i + j ] = CLIENTID;
					j++;
				}
				break;
			}
		}
	}
	else if( zone == 'D' )
	{
		for( i = 0; i < 230; i++ )
		{
			if( ptr->zoneD[ i ] == 0 )
			{
				j = 0;
				while( j < positions )
				{
					ptr->zoneD[ i + j ] = CLIENTID;
					j++;
				}
				break;
			}
		}
	}
}

/* I sinartisi seatCheck elenxei analoga me tin zwni pou exei
 * epileksei o xristis na ginei kai i katallili anazitisi
 * xrisimopoiontas tis sinartiseis searchInA, klp 
 * Epistrefei 1 an vrike kai 0 an oxi */
int seatCheck( char zone, int positions, theatre *ptr )
{
	int found; 
               
	if ( zone == 'A' )
	{
		found = searchInA( positions, ptr );
	}
	else if( zone == 'B' )
	{
		found = searchInB( positions, ptr );
	}
	else if( zone == 'C' )
	{
		found = searchInC( positions, ptr );
	}
	else if( zone == 'D' )
	{
		found = searchInD( positions, ptr );
	}
         
	return found;
}
				

/* Diavazoume tis theseis apo ton client */
int readPositions( int connfd )
{
	int positions;
               
	read( connfd, &positions, sizeof( int ) );
	return positions;
}
      
/* Diavazoume tin zwni apo ton client */	  
char readZone( int connfd )
{
	char zone;
               
	read( connfd, &zone, sizeof( char ) );
	return zone;
}
         
 
/* Arxikopoioume tis theseis ton zwnwn-pinakwn A-B-C-D se 0 ( diladi einai adies ) */ 
void initializeStruct( theatre *ptr )
{
	int i;
               
	// Zone A
	for( i = 0; i < 100; i++ )
	{
		ptr->zoneA[ i ] = 0;
	}
               
	// Zone B
	for( i = 0; i < 130; i++ )
	{
		ptr->zoneB[ i ] = 0;
	}
               
	// Zone C
	for( i = 0; i < 180; i++ )
	{
		ptr->zoneC[ i ] = 0;
	}
               
	// Zone D
	for( i = 0; i < 230; i++ )
	{
		ptr->zoneD[ i ] = 0;
	}
	
	// Arxikopoihsh pinaka metaforwn 
	for( i = 0; i < 100; i++ )
	{
		str->accountTransfer[ i ] = 0;
	}
	
	str->companyAccount = 0;
	str->sunXronosAnamonis = 0;
	str->sunXronosEksip = 0;
	str->failOrders = 0;
	str->cardInvalidTimes = 0;
}
   
/* Anazitisi thesewn stin zwni A */
int searchInA( int positions, theatre *ptr )
{
	int i;
	int count = 0; /* Apothikeuoume to plithos ton eleftherwn thesewn pou vrikame */
               

	for( i = 0; i < 100; i++ )
	{
		if( ptr->zoneA[ i ] == 0 )
		{
			count++;
		}
	}
     
	/* Elenxoume an to plithos ton eleftherwn thesewn einai megalitero H iso 
	 * apo to plithos twn thesewn pou thelei na kleisei o client */ 
	if ( count >= positions ) 
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
    
/* Anazitisi thesewn stin zwni C */	
int searchInB( int positions, theatre *ptr )
{
	int i;
	int count;
               
	for( i = 0; i < 130; i++ )
	{
		if( ptr->zoneB[ i ] == 0 )
		{
			count++;
		}
	}
               
	if( count >= positions )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
 
/* Anazitisi thesewn stin zwni C */
int searchInC( int positions, theatre *ptr )
{
	int i;
	int count;
               
	for( i = 0; i < 180; i++ )
	{
		if( ptr->zoneC[ i ] == 0 )
		{
			count++;
		}
	}
               
	if ( count >= positions )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/* Anazitisi thesewn stin zwni D */         
int searchInD( int positions, theatre *ptr )
{
	int i;
	int count;
               
	// zone d
	for( i = 0; i < 230; i++ )
	{
		if( ptr->zoneD[ i ] == 0 )
		{
			count++;
		}
	}
               
	if ( count >= positions )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
