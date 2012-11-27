/*----------------------------------------------------------------------------*\
|Remote OpenGL interface used by the host application to send OpenGL commands  |
|to the visualization wall using GLPipes. Reads a config file to set up pipes. |
|                                                                              |
|Stewart Hall                                                                  |
|11/26/2012                                                                    |
\*----------------------------------------------------------------------------*/

#define _CRT_SECURE_NO_WARNINGS

#include "GLPipe.h"

#include <gl\gl.h>
#include <gl\glu.h>

//The size of the buffer to hold command and arguments
#define BUFFER_SIZE 10485760

class RGLInterface
{
private:
	//Total number of nodes to connect to
	int num_nodes;

	//Pipes for each node
	GLPipe **pipes;

	//The buffer that holds queued data
	char *buffer;

	//Buffer pointer for pushing data
	unsigned int buffer_pointer;

	//Total dimensions of host application
	int width, height;

public:
	RGLInterface(char *configFile);
	~RGLInterface();

	//Called to set up a connection to the nodes
	int initialize(char *address);

	//Called to close connections to nodes and clean up
	void cleanUp();

	//Pushes a command to the buffer
	void pushCommand(int id);

	//Pushes a GLFloat value to the buffer
	void pushGLfloat(GLfloat value);

	//Pushes a GLBitfield value to the buffer
	void pushGLbitfield(GLbitfield value);

	//Pushes a GLenum value to the buffer
	void pushGLenum(GLenum value);

	//Sends data in the buffer over all pipes
	void sendCommand();

	//Sends a syncronization packet telling the nodes to swap buffers
	void sendSync();

	//----------------
	//OpenGL functions
	//----------------
	void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void glClear(GLbitfield mask);
	void glLoadIdentity(void);
	void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
	void glBegin(GLenum mode);
	void glEnd(void);
	void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
};
