
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>

#include <glib.h>

//#define DEBUG

#include "server.h"
#include "gui.h"

#define STARTMESSAGE "* Stage-"VERSION" *"
#define STOPMESSAGE  "Stage finished."

// Signal catchers ---------------------------------------------------

void catch_signal( int signum )
{
  switch( signum )
    {
    case SIGINT:
      puts( "\nInterrupt signal." );
      stg_quit_request();
      break;

    case SIGQUIT:
      puts( "\nQuit signal." );
      stg_quit_request();
      break;
      
    case SIGPIPE: // TODO - handle SIGPIPE properly?
      puts( "\nBroken pipe signal." ); fflush(stdout);
      break;

    default:
      PRINT_WARN1( "unhandled signal(%d)", signum );
      break;
    }
}


void catch_exit( void )
{
  // write a goodbye message on exit
  puts( STOPMESSAGE );
}


int install_signal_catchers( void )
{
  atexit( catch_exit );
  
  if( signal( SIGINT, catch_signal ) == SIG_ERR )
    {
      PRINT_ERR( "error setting interrupt signal catcher" );
      exit( -1 );
    }
  
  if( signal( SIGQUIT, catch_signal ) == SIG_ERR )
    {
      PRINT_ERR( "error setting quit signal catcher" );
      exit( -1 );
    }

  if( signal( SIGPIPE, catch_signal ) == SIG_ERR )
    {
      PRINT_ERR( "error setting quit signal catcher" );
      exit( -1 );
    }

  /*if( signal( SIGALRM, catch_signal ) == SIG_ERR )
    {
      PRINT_ERR( "error setting timer signal catcher" );
      exit( -1 );
      }
  */

  return 0; // ok
}

int main( int argc, char* argv[] )
{
  printf( "%s ", STARTMESSAGE ); fflush(stdout);

  int server_port = STG_DEFAULT_SERVER_PORT;
 
  // parse args
  int a;
  for( a=1; a<argc; a++ )
    {
      if( strcmp( "-p", argv[a] ) == 0 )
	{
	  server_port = atoi( argv[++a] );
	  printf( " [port %d]", server_port );
	  continue;
	}
    }

  install_signal_catchers();

  server_t* server = NULL;
  
  if( !(server = server_create( server_port )) )
    stg_err( "failed to create server" );
  
  gui_startup( &argc, &argv );
  
  // finish startup line
  puts( "\n* Ready." );
  

#ifdef DEBUG
  struct timeval tv1, tv2;
#endif
  
  while( !stg_quit_test() )
    {	  
      //puts( "Loop start" );

#ifdef DEBUG
      gettimeofday( &tv1, NULL );
#endif

      server_poll( server ); // read from server port and
			     // clients. Includes a minimum poll()
			     // sleep time
#ifdef DEBUG
      gettimeofday( &tv2, NULL );  
      printf( " server poll time %.6f\n",
	      (tv2.tv_sec + tv2.tv_usec / 1e6) - 
	      (tv1.tv_sec + tv1.tv_usec / 1e6) );	    
#endif
      
      
      
#ifdef DEBUG
      gettimeofday( &tv1, NULL );
#endif
      server_update_worlds( server );
      server_update_subs( server );
	
#ifdef DEBUG
	gettimeofday( &tv2, NULL );
	printf( " update worlds & subs time %.6f\n",
		(tv2.tv_sec + tv2.tv_usec / 1e6) - 
		(tv1.tv_sec + tv1.tv_usec / 1e6) );	    
#endif

#ifdef DEBUG
      gettimeofday( &tv1, NULL );
#endif

      gui_poll();

#ifdef DEBUG
	gettimeofday( &tv2, NULL );
	printf( " gui poll time %.6f\n",
		(tv2.tv_sec + tv2.tv_usec / 1e6) - 
		(tv1.tv_sec + tv1.tv_usec / 1e6) );	    
#endif
    }
  
  // todo - this causes a segfault - fix
  //gui_shutdown();

  server_destroy( server );
  
  puts( "Stage exiting main loop" );

  return 0; // done
}
