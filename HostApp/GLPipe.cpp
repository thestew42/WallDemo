/*----------------------------------------------------------------------------*\
|Establishes and maintains a connection to the GLNode and sends OpenGL commands|
|to the node.                                                                  |
|                                                                              |
|Stewart Hall                                                                  |
|11/26/2012                                                                    |
\*----------------------------------------------------------------------------*/

#include "GLPipe.h"

//Constructor
GLPipe::GLPipe(int h_width, int h_height, int width, int height, int x_off, int y_off, int x_loc, int y_loc, int dev, int n_port, char *name)
{
	fullscreen = FALSE;
	host_width = h_width;
	host_height = h_height;
	win_width = width;
	win_height = height;
	x_offset = x_off;
	y_offset = y_off;
	x_location = x_loc;
	y_location = y_loc;
	device_id = dev;
	port = n_port;
	node_identifier = name;
}

//Destructor
GLPipe::~GLPipe()
{
	delete node_identifier;
}

//Establishes a connection to the GLNode
int GLPipe::connectPipe(char *address)
{
	int length = 0;

	//Create the socket
	pipe_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(pipe_sock == INVALID_SOCKET) {
		printf("Error %d occurred!\n",  WSAGetLastError());
		return - 1;
	}

	//Connect to the node
	sockaddr_in anews;
	anews.sin_port = htons(port);
	anews.sin_addr.S_un.S_addr = inet_addr(address);
	anews.sin_family = AF_INET;
	if(connect(pipe_sock, (sockaddr*)&anews, sizeof(anews)) == SOCKET_ERROR) {
		printf("Error %d occurred!\n",  WSAGetLastError());
		return -1;
	}
	
	//Send the name of this pipe's target node for validity check
	if((length = send(pipe_sock, node_identifier, 256, 0)) != 256) {
		if(length == SOCKET_ERROR)
			printf("Error %d occurred!\n",  WSAGetLastError());
		else
			printf("Connection was terminated unexpectedly\n");
			
		return -1;
	}

	return 0;
}

//Close the pipe
void GLPipe::closePipe()
{
	closesocket(pipe_sock);
}

//Send a command over the pipe
int GLPipe::sendCommand(char *buffer, unsigned int length)
{
	int send_length;

	//Send the name of this pipe's target node for validity check
	if((send_length = send(pipe_sock, buffer, length, 0)) != length) {
		if(send_length == SOCKET_ERROR)
			printf("Error %d occurred!",  WSAGetLastError());
		else
			printf("Connection was terminated unexpectedly");
			
		return -1;
	}

	return 0;
}

//Print configuration information about the pipe
void GLPipe::printStatus()
{
	printf("GLNode configuration and status:\n");
	printf("\tHost application dimensions: %dpx x %dpx\n", host_width, host_height);
	printf("\tWindow dimensions: %dpx x %dpx\n", win_width, win_height);
	printf("\tWindow offset: %dpx, %dpx\n", x_offset, y_offset);
	printf("\tWindow location: %dpx, %dpx\n", x_location, y_location);
	printf("\tUse graphics device: %d\n", device_id);
	printf("\tListen on port: %d\n", port);
}
