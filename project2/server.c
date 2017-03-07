#include "MyHeader.h" /* for user-defined constants */
 
                       
/* Size of the request queue. */
#define LISTENQ 100       

 
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


// Global Variables 
int CLIENTID = 0;        /* Arithmos pelati */
int theatreAccount = 0;  /* Logariasmos theatrou */
int totalOrders = 0;     /* Sunolikos arithmos paraggeliwn */
int count = 0;           /* Metritis gia ton pinaka metaforwn xrimatwn */
theatre *str;          /* Deiktis pros tin struct */
time_t start_time;       /* Timer */

/* Dhmiourgia mutexes, condition variables kai xrhsimwn metritwn */
pthread_mutex_t til_mutex;
pthread_mutex_t save_mutex;
pthread_mutex_t search_mutex;
pthread_mutex_t trap_mutex;
pthread_cond_t trap_cond;
pthread_cond_t til_cond;
int til_count = 1;
int trap_count = 1;

/* Id pelati */
int id=0;


/* Function prototypes */

double calcPercentage( int totalOrders, int failOrders );       /* Pososto apotiximenwn/epituximenwn prospatheiwn */
 
int readPositions( int );       	/* Diavazei ton arithmo thesewn pou epilexthikan apo ton pelati*/

int isFull( theatre *ptr );     	/* Elegxoume an exei gemisei to theatro*/

int seatCheck( char zone, int positions, theatre *ptr );        /* Elegxei tis theseis tou pinaka		thesewn	ths zwnhs pou epilextike */ 												   
int searchInA( int, theatre * );      /* Elegxei an yparxoun adeies theseis ston pinaka zwnhs A */
int searchInB( int, theatre * );      /* Elegxei an yparxoun adeies theseis ston pinaka zwnhs B */  
int searchInC( int, theatre * );      /* Elegxei an yparxoun adeies theseis ston pinaka zwnhs C */ 
int searchInD( int, theatre * );      /* Elegxei an yparxoun adeies theseis ston pinaka zwnhs D */  

int chargeSystem( char, int );        /* Ypologismos xrewshs gia kathe pelati */

char readZone( int);       /* Diavazei th zwnh pou epilexthike apo ton pelati */
 
void initializeStruct( theatre * );    		/* Arxikopoioume tous pinakes thesewn. Enas pinakas gia kathe zwnh */

void reservePositions( char zone, int positions, theatre *ptr );	/* Ekxwroume tis kratiseis se adeies theseis twn pinakwnthesewn */
 
void sendMessage( int type, int connfd, theatre *ptr );         /* Stelnei ston client mia metavliti type kai tis zwnes etsi wste o client na ektipwsei to katallilo minima */
 
void printZones( theatre *ptr );        /* Ektupwsh pinaka thesewn */

/* Synarthseis nhmatwn */
void *til_thread(void *);
void *trap_thread(void *);


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


/* sinartisi gia to SIGINT */
/* An steiloume shma na kleisei o server (patwntas CTRL+C) tote tha emfanistoun ta telika apotelesmata */
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

/* Start of main program */
int main( int argc, char **argv )
{	


	struct sockaddr_un cliaddr, servaddr;
	int listenfd;         
	int connfd;
	pthread_t tilef_thread;	/* Nhma tilefwnitwn */
	socklen_t clilen;
	int i;

	/* Arxikopoihsh twn mutexes kai condition variables */
	pthread_mutex_init(&til_mutex, NULL);
	pthread_mutex_init(&save_mutex, NULL);
	pthread_mutex_init(&search_mutex, NULL);
	pthread_mutex_init(&trap_mutex, NULL);
	pthread_cond_init(&til_cond, NULL);
	pthread_cond_init(&trap_cond,NULL);	
	

	/* Desmeuoume mnhmh gia to struct */
	str = malloc(sizeof(theatre));	
	
	/* Arxikopoihsh pinakwn thesewn kathe zwnhs */
	initializeStruct( str );


	/* Dilwsh signal handler gia thn alarm */
	signal( SIGALRM, myalarm );

	/* Signal pou termatizei ton server kai emfanizei apotelesmata */
	signal(SIGINT, sig_server);	

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

	clilen = sizeof( cliaddr );
         
	/* Ksekinaei to alarm */
	alarm( 30 );

	/* Copy next request from the queue to connfd and remove it from the queue. */
	while (connfd = accept( listenfd, ( struct sockaddr * ) &cliaddr, &clilen ))
	{
	
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

		/* Dhmiourgia nhmatos tilefwniti */
		if(pthread_create(&tilef_thread, NULL, til_thread, &connfd))
		{
			fprintf(stderr,"Error creating thread\n");
			return 0;
		}
		
		
		pthread_detach(tilef_thread);
		
	}
	
	/* Kleinoume to socket tou server */
	close(connfd);	
	return 0;
}

/* Nhma tilefwniti */
void *til_thread(void *connec)
{
	// timers
	time_t time_an;
	time_t time_ek;

	pthread_t trapeza_thread;	/* Nhma trapezas */
	int positions;			/* O arithmos twn thesewn pou epelekse o client */
	char zone;			/* I zwni pou epelekse o client */
	int card;			/* I timi tis kartas pou diavastike apo ton client */
	int charge;			/* Xrewsi */
	int seatFound = 0;		/* Apothikeuoume an vrikame thesi */
	int connfd = *((int *)connec);	/* Type-casting se int */
	int i;
	float xronosAnamonis; /* Xronos anamonis ana pelati */
	float xronosEksip;    /* Xronos eksipiretisis ana pelati */

	
	/* Ksekinaei na metraei o timer start_time */
	start_time = time( NULL );

	/* Auksanetai to id pelati */
	id++;

	/* Ksekinaei na metraei o timer anamonis */
	time_an = time( NULL );

	printf("O pelatis %d epixeirei na kanei connect\n", id);

	pthread_mutex_lock(&til_mutex);		
	/* An einai apasxolimenoi kai oi 10 tilefwnites, oi upoloipoi pelates pou sundeontai mpainoun sthn 		anamonh */
	if(til_count>10)
	{
		printf("O pelatis %d mpike sthn anamonh\n", id);
		pthread_cond_wait(&til_cond,&til_mutex);	
		printf("Enas pelatis vgike apo thn anamonh\n");
	}
	til_count++;	/* Auksanoume ton counter tilefwnitwn */
	pthread_mutex_unlock(&til_mutex);
	

	/* Ypologismos xronou anamonis ana client */
	xronosAnamonis = time( NULL ) - time_an;

							
	/* Prostithete ston sunoliko xrono anamonis */
	str->sunXronosAnamonis += xronosAnamonis;
	
	/* To save_mutex kleidwnei */
	pthread_mutex_lock(&save_mutex);

	/* Kathisterisi 6 deuteroleptwn */
	sleep(1);

	/* Auksanoume to id tou client. Diaforetiko kai aneksartito apo th metavliti 'id'. Kaname 2 id giati 		xrhsimopoioume to prwto prin to condition variable kai me mono ena id ta pragmata mperdeuontai otan 		kapoios pelatis vgainei apo thn anamonh. To apotelesma einai swsto. */
	CLIENTID++;

	/* Neos client dimiourgithike opote auksanoume tis paraggelies */
	totalOrders++;

	write(connfd, &CLIENTID, sizeof( int ) );
	printf("Client %d is proceeding with their order\n", CLIENTID);

 
	/* Ksekinaei na metraei o timer eksipiretisis */
	time_ek = time( NULL );


	/* Read positions apo ton client */
	positions = readPositions( connfd );
	
        	                      
	/* Read zone apo ton client */
	zone = readZone( connfd );
          
	/* To save_mutex ksekleidwnei */  
        pthread_mutex_unlock(&save_mutex);      

                                
	/* Read card from client */
	card = readCard( connfd ); 
	
	
	/* Dhmiourgia nhmatos trapezas */
	if(pthread_create(&trapeza_thread, NULL, trap_thread, &card))
		{
			fprintf(stderr,"Error creating thread\n");
			return 0;
		}	
	

	/* To nhma tilefwniti perimenei na teleiwsei to nhma-trapeza protou sunexisei na ekteleitai */
	if(pthread_join(trapeza_thread, NULL))
	{
		fprintf(stderr,"Error joining thread\n");
		return 0;
	}

	pthread_detach(trapeza_thread);
	
	/* Kleidwnei to search_mutex */
	pthread_mutex_lock(&search_mutex);

	/* Elegxos gia eleftheres theseis stin sigkekrimeni zwni
	p epelekse o client */	
	seatFound = seatCheck( zone, positions, str );

	/* An den vroume theseis auksanoume tis apotiximenes paraggelies 
	kai stelnoume minima ston client */	
	if( seatFound == 0 )
	{
		str->failOrders++;
		sendMessage( 3, connfd, str );		
									
	}
                                    
	/* An vrike eleftheres theseis kai i pistwtiki
	einai egkuri proxarame se kratisi thesewn */
	if( str->cardValid == 1 && seatFound == 1 )
	{
			
		/* Katoxirwsi thesewn */
		reservePositions( zone, positions, str );
				
		/* Sistima xrewsis */
		charge = chargeSystem( zone, positions );
        	                               
		/* Update company account */
		str->companyAccount += charge;
                                       
		/* Stelnoume minima epitixias ston client */
		sendMessage( 1, connfd, str );
		printf("O pelatis %d oloklirwse thn paraggelia\n", 	CLIENTID);
	}
	
	/* Meiwnoume ton counter tilefwnitwn */
	til_count--;

	/* An kapoios tilefwnitis apeleutherwthei stelnetai shma na vgei apo thn anamonh enas pelaths pou 		perimenei */
	if(til_count<10)
	{
		pthread_cond_signal(&til_cond);
	}
	
	/* Ksekleidwnei to search_mutex */
	pthread_mutex_unlock(&search_mutex);

	/* Ypologismos xronou eksipiretisis */
	xronosEksip = time( NULL ) - time_an;
 
	/* Prostithetai ston sunoliko xrono eksipiretisis */
	str->sunXronosEksip += xronosEksip;

                  
	/* Elegxoume an gemisan oloi oi theseis tou pinaka. An nai 
	tote ektipwnoume ta telika apotelesmata */
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
		/* Stelnoume to minima oti oles oi theseis exoun 				gemisei */
		sendMessage( 4, connfd, str );
		exit( 0 );
	}
	
	/* Termatismos nhmatos tilefwniti */
	pthread_exit(NULL);
}


/* Nhma trapezas */
void *trap_thread(void *terminal)
{
	int card = *((int*)terminal);

	/* Kleidwnei to trap_mutex */
	pthread_mutex_lock(&trap_mutex);

	/* An idi 4 tilefwnites elegxoun thn egkurothta kartas tote oi upoloipoi mpainoun sthn anamonh */
	if(trap_count>4)
	{
		printf("O tilefwnitis elegxei thn karta\n");
		pthread_cond_wait(&trap_cond,&trap_mutex);
		printf("H karta elegxthike\n");
	}
	trap_count++;	/* O metritis termatikwn auksanetai */

	/* Ksekleidwnei to trap_mutex */
	pthread_mutex_unlock(&trap_mutex);

	           
	/* An i karta einai 1 simainei oti einai egkuri, opote apothikeuoume to 1 sto cardValid pou exoume stin 	struct mas */
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
	
	/* O metritis twn termatikwn meiwnetai */
	trap_count--;

	/* An eleutherwthei ena termatiko/trapeziths tote vgainei enas tilefwnitis apo thn anamonh */
	if(trap_count<4)
	{
		pthread_cond_signal(&trap_cond);
	}
	
	/* Termatismos nhmatos trapezas */
	pthread_exit(NULL);
	
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
            
	/* Otan to count ginei 640 simanei oti oles oi theseis exoun katoxirwthei */
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

/* Ypologizoume tin xrewsi tou kathe client analoga me tin zwni pou exei epileksei */
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

/* Edw ginete i kratisi twn thesewn. I logiki einai i eksis. Psaxnoume ton pinaka mexri na vroume tin prwti eleftheri thesi. Epeita kanoume mia defteri epanalipsi me metriti ton arithmo twn thesewn p exei dwsei o client kai apothikeoume stin katallili zwni to CLIENTID */
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

/* I sinartisi seatCheck elenxei analoga me tin zwni pou exei epileksei o xristis na ginei kai i katallili anazitisi xrisimopoiontas tis sinartiseis searchInA, klp. Epistrefei 1 an vrike kai 0 an oxi */
int seatCheck( char zone, int positions, theatre *ptr )
{
	int found; 
               	
	printf("\n");	
	
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
	int Acount = 0; /* Apothikeuoume to plithos ton eleftherwn thesewn pou vrikame */
               

	for( i = 0; i < 100; i++ )
	{
		if( ptr->zoneA[ i ] == 0 )
		{
			Acount++;
		}
	}
     
	/* Elegxoume an to plithos ton eleftherwn thesewn einai megalitero h iso apo to plithos twn thesewn pou 	thelei na kleisei o client */ 
	if ( Acount >= positions ) 
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
	int Bcount = 0;
             
	for( i = 0; i < 130; i++ )
	{
		
		if( ptr->zoneB[ i ] == 0 )
		{
			Bcount++;
		}
	}
               
	if( Bcount >= positions )
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
	int Ccount = 0;
               
	for( i = 0; i < 180; i++ )
	{
		
		if( ptr->zoneC[ i ] == 0 )
		{
			Ccount++;
		}
	}
		   
	if ( Ccount >= positions )
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
	int Dcount = 0;
               
	// zone d
	for( i = 0; i < 230; i++ )
	{
		
		if( ptr->zoneD[ i ] == 0 )
		{
			Dcount++;
		}
	}
               
	if ( Dcount >= positions )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}










