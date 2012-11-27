/*----------------------------------------------------------------------------*\
|The main class for a node. Listens for a connection from the host application |
|and opens a window when connection is established. Receives and forwards      |
|OpenGL commands from the host.                                                |
|                                                                              |
|Stewart Hall                                                                  |
|11/19/2012                                                                    |
\*----------------------------------------------------------------------------*/

#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>

//The size of the buffer to hold command and arguments
#define BUFFER_SIZE 10485760

#define PI 3.14159265f

//Declaration For WndProc
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

class GLNode {
private:
    //Dimensions of this window
    int win_width, win_height;

	//Is the window fullscreen
	BOOL fullscreen;

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

    //Private GDI Device Context
    HDC hDC;

    //Permanent Rendering Context
    HGLRC hRC;

    //Holds Our Window Handle
    HWND hWnd;

    //Holds The Instance Of The Application
    HINSTANCE hInstance;

	//Socket to read commands over
	SOCKET node_sock;

	//Syncronization flag
	BOOL sync;

    //Command line arguments specify config file and this node's identifier
    char *configFile;
    char *nodeIdentifier;

	//Buffer that holds arguments
	char *buffer;
	unsigned int buffer_pointer;
	unsigned int buffer_size;

    //Read node attributes from a config file
    void readConfiguration();

public:
    GLNode(char *i_configFile, char *i_nodeIndentifier);
	~GLNode();

    //Start listening for a connection from the host
    void startListening();

    //Accept a connection and open the window
    void acceptConnection();

	//Receives an OpenGL command and runs it
	int receiveCommand();

	//Prepares the internal buffer for arguments
	void prepareBuffer(unsigned int length);

	//Gets a GLfloat from the buffer
	void getGLfloat(GLfloat *value);

	//Gets a GLenum value from the buffer
	void getGLenum(GLenum *value);

	//Gets a GLbitfield from the buffer
	void getGLbitfield(GLbitfield *value);

    //Creates the window
    BOOL createWindow(char* title, int width, int height, int bits);

	//Kills the window
	GLvoid KillGLWindow(GLvoid);

	//Resizes the OpenGL canvas
	GLvoid ReSizeGLScene(GLsizei width, GLsizei height);

	//Initializes OpenGL
	int InitGL(GLvoid);

    //Idle wait for OpenGL commands
    void mainLoop();

    //Prints out configuration data and connection info
    void printStatus();

	//----------------
	//OpenGL functions
	//----------------
	void _glClearColor();
	void _glClear();
	void _glLoadIdentity();
	void _glTranslatef();
	void _glBegin();
	void _glEnd();
	void _glVertex3f();
	void _glColor3f();
	void _glRotatef();
};
