/*
 * displays a spectrogram of the input and mirrors the input to the output.
 *
 * uses gtk to draw the window
 *
 *
 * input: read from stdin
 * output: written to stdout
 *
 */

#include <pthread.h>
#include <gtk/gtk.h>
#include <rfftw.h>
#include <glib.h>

#include "common.h"
const int N = 4096;
const int SR = 44100;

pthread_t gtk_thread;
GdkPixmap* pixmap;
GtkWidget *widget;
rfftw_plan plan;

buffer_t *buffer[2];
int current_buffer = 0;


#define OFFLINE_BUFFER buffer[current_buffer^1]
#define ONLINE_BUFFER buffer[current_buffer]


// equal loudness curves
float rc(int i) {
  double f = (double)i * SR/N;
  //  return 1.0;
  return ( 12200*12200*f*f /((f*f+20.62)*(f*f+12200*12200)) );
}

// equal loudness curves
float ra(int i) {

  double f = idx_to_freq(i, SR, N);
  double a = 12200;
  double b = 20.6;
  double c = 107.7;
  double d = 737.9;

  return a*a * f*f*f*f / ( (f*f + b*b) * (f*f + a*a)*sqrt(f*f+c*c)*sqrt(f*f + d*d) ); 
}


int idx_to_x(int i) {
  return 40*log(idx_to_freq(i, SR, N))/log(2);
}

int pow_to_y(int i,float re, float im) {
  return 40.0*log(ra(i) *sqrt(re*re + im*im))/log(2);
}


/*
 * Terminate the main loop.
 */
static void
on_destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}


/* Redraw the screen from the backing pixmap */
static gint
expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  int i;
  fftw_real in[N], out[N];
  
  gdk_draw_rectangle (pixmap,
					  widget->style->white_gc,
					  TRUE,
					  0, 0,
					  widget->allocation.width,
					  widget->allocation.height);


  for (i=0; i<N; i++ ) in[i] = ((fftw_real)OFFLINE_BUFFER[i] -128.0) / 128.0;

  rfftw_one(plan, in, out);

  for (i=1; i<N/2; i++ ) {
	gdk_draw_line (pixmap,
				   widget->style->black_gc,
				   10 +idx_to_x(i), widget->allocation.height-5,
				   10 +idx_to_x(i), widget->allocation.height - pow_to_y(i,out[i],out[N-i])-5);
  }
  
  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  pixmap,
                  0, 0, 0, 0,
                  event->area.width, event->area.height);

  return FALSE;
}



static gint
configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
  if (pixmap)
	gdk_pixmap_unref(pixmap);

  pixmap = gdk_pixmap_new(widget->window,
						  widget->allocation.width,
						  widget->allocation.height,
						  -1);
  
  gdk_draw_rectangle (pixmap,
					  widget->style->white_gc,
					  TRUE,
					  0, 0,
					  widget->allocation.width,
					  widget->allocation.height);
  
  return TRUE;
}

void* gtk_thread_loop(void* arg) {
  GtkWidget *window;


  g_thread_init (NULL);
  gdk_threads_init ();
  
  gtk_init (NULL, NULL);
  

  gdk_threads_enter ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_container_set_border_width (GTK_CONTAINER (window), 20);

  gtk_window_set_title (GTK_WINDOW (window), (const char*) arg);

  gtk_window_set_default_size (GTK_WINDOW (window), idx_to_x(N/2)+30, 500);

  g_signal_connect (G_OBJECT (window), "destroy",
					G_CALLBACK (on_destroy), NULL);
  
  widget = gtk_drawing_area_new();
  

  gtk_signal_connect (GTK_OBJECT(widget),"configure_event",
                      (GtkSignalFunc) configure_event, NULL);

  gtk_signal_connect (GTK_OBJECT (widget), "expose_event",
                      (GtkSignalFunc) expose_event, NULL);
  
  gtk_signal_connect(GTK_OBJECT(window), "destroy", 
  					 (GtkSignalFunc) on_destroy, NULL);

  gtk_widget_set_events (widget, GDK_EXPOSURE_MASK
                         | GDK_LEAVE_NOTIFY_MASK
                         | GDK_POINTER_MOTION_MASK
                         | GDK_POINTER_MOTION_HINT_MASK);

  gtk_container_add (GTK_CONTAINER (window), widget);  
  gtk_widget_show_all (window);
  gtk_main ();
  gdk_threads_leave ();
  return NULL;
  
}


int main(int argc, char** argv) {

  int rd;
  long bytes = 0;
  	int i;

  if (argc != 2) 
	die("usage: hist histogramtitle");


  pthread_create(&gtk_thread, NULL, gtk_thread_loop, (void*)argv[1]); 
  plan = rfftw_create_plan(N, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);

  buffer[0] = malloc ( N*sizeof(buffer_t));
  buffer[1] = malloc ( N*sizeof(buffer_t));

  
  sleep(1);

  while (GTK_IS_WIDGET (widget) ) {

#if 0
	// read sample
	rd = read(0, &ONLINE_BUFFER, N* sizeof(buffer_t)); 


	// eof
	if ( rd != N*sizeof(buffer_t) )  {
	  fprintf(stderr, "%li bytes processed, %d read, exiting (%s)\n", bytes, rd, argv[1]);
	  return 0;
	}

#else

	for (i=0; i<N; i++)
	  rd = read(0, &ONLINE_BUFFER[i], sizeof(buffer_t)); 

	if ( rd < 1 )  {
	  fprintf(stderr, "%li bytes processed, %d read, exiting (%s)\n", bytes, rd, argv[1]);
	  return 0;
	}

#endif


	gdk_threads_enter();
	//gtk_widget_queue_draw (widget);	
	gtk_widget_draw(widget, NULL);
	gdk_threads_leave();

#if 0
	write(1, &ONLINE_BUFFER, rd);
	bytes += rd;
#else
	write(1, &ONLINE_BUFFER, N* sizeof(buffer_t));
	bytes += rd;
#endif

	current_buffer ^= 1;
  }

  pthread_join(gtk_thread, NULL);
  rfftw_destroy_plan(plan);
  
  free(buffer[0]);
  free(buffer[1]);

  return 0;
}
