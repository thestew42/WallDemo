/*----------------------------------------------------------------------------*\
|Establishes and maintains a connection to the GLNode and sends OpenGL commands|
|to the node.                                                                  |
|                                                                              |
|Stewart Hall                                                                  |
|11/26/2012                                                                    |
\*----------------------------------------------------------------------------*/

#define _CRT_SECURE_NO_WARNINGS

#ifndef CAPTUREDLL
#include <windows.h>
#include <stdio.h>
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#define NULL 0
typedef int BOOL;
typedef unsigned int SOCKET;
#endif

class GLPipe
{
private:
	//Is the window fullscreen
	BOOL fullscreen;

	//Window dimensions
	int win_width, win_height;

	//Total dimensions of the host application
	int host_width, host_height;

	//Offset of this window in the total resolution of the host
	int x_offset, y_offset;

	//Location of this window in screen space
	int x_location, y_location;

	//ID of the graphics device to use
	int device_id;

	//Port to listen on
	int port;

	//Name of this node
	char *node_identifier;

	//Socket to communicate over
	SOCKET pipe_sock;

public:
	GLPipe(int h_width, int h_height, int width, int height, int x_off, int y_off, int x_loc, int y_loc, int dev, int n_port, char *name);
	~GLPipe();

	//Connects to the node
	int connectPipe(char *address);

	//Closes the connection to the node
	void closePipe();

	//Sends data in the supplied buffer to the node
	int sendCommand(char *buffer, unsigned int length);

	//Prints out configuration data and connection info
	void printStatus();
};
