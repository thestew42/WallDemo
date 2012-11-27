/*----------------------------------------------------------------------------*\
|Remote OpenGL interface used by the host application to send OpenGL commands  |
|to the visualization wall using GLPipes. Reads a config file to set up pipes. |
|                                                                              |
|Stewart Hall                                                                  |
|11/26/2012                                                                    |
\*----------------------------------------------------------------------------*/

#include "RGLInterface.h"

//Initializes an interface with a config file
RGLInterface::RGLInterface(char *configFile)
{
	char *line = new char[256];
    char *tag;
    char *number;

	//Number of nodes that have been parsed
	int nodes_read = 0;

	//Temporary node properties
	int win_width, win_height;
	int x_offset, y_offset;
	int x_location, y_location;
	int device_id;
	int port;

	num_nodes = 0;
	pipes = NULL;
	buffer = NULL;
	buffer_pointer = 0;
    FILE *fp = fopen(configFile, "r");

    if(fp) {
        //Read each line in the config file sequentially
        while(!feof(fp)) {
            //Read a line
            fgets(line, 256, fp);

            //Extract the tag on this line
            tag = strtok(line, " :");

            //Look for global variables or this node's sub-section
            if(!strcmp(tag, "totalWidth")) {
                number = strtok(NULL, " :");
                sscanf(number, "%d", &width);
            } else if(!strcmp(tag, "totalHeight")) {
                number = strtok(NULL, " :");
                sscanf(number, "%d", &height);
			} else if(!strcmp(tag, "nodes")) {
                number = strtok(NULL, " :");
                sscanf(number, "%d", &num_nodes);

				//Make room for the pipes
				pipes = new GLPipe*[num_nodes];
            } else if(nodes_read < num_nodes) {
				//Copy the node's name
				char *name = new char[256];
				strcpy(name, tag);

                //Read the next node's properties
                while(!feof(fp)) {
                    //Read a line
                    fgets(line, 256, fp);

                    //Extract the tag on this line
                    tag = strtok(line, " :");
                    if(!strcmp(tag, "width")) {
                        number = strtok(NULL, " :");
                        sscanf(number, "%d", &win_width);
                    } else if(!strcmp(tag, "height")) {
                        number = strtok(NULL, " :");
                        sscanf(number, "%d", &win_height);
                    } else if(!strcmp(tag, "xOffset")) {
                        number = strtok(NULL, " :");
                        sscanf(number, "%d", &x_offset);
                    } else if(!strcmp(tag, "yOffset")) {
                        number = strtok(NULL, " :");
                        sscanf(number, "%d", &y_offset);
                    } else if(!strcmp(tag, "xLocation")) {
                        number = strtok(NULL, " :");
                        sscanf(number, "%d", &x_location);
                    } else if(!strcmp(tag, "yLocation")) {
                        number = strtok(NULL, " :");
                        sscanf(number, "%d", &y_location);
                    } else if(!strcmp(tag, "device")) {
                        number = strtok(NULL, " :");
                        sscanf(number, "%d", &device_id);
                    } else if(!strcmp(tag, "address")) {
                        number = strtok(NULL, " :");
                        sscanf(number, "%d", &port);
                    } else {
                        //Construct this node's pipe
						pipes[nodes_read] = new GLPipe(
							width,
							height,
							win_width,
							win_height,
							x_offset,
							y_offset,
							x_location,
							y_location,
							device_id,
							port,
							name);
						pipes[nodes_read]->printStatus();

						nodes_read++;
						break;
                    }
                }
            }
        }
    } else {
        printf("Error opening config file\n");
    }

    delete line;
}

//Destructor
RGLInterface::~RGLInterface()
{
	if(pipes)
		delete[] pipes;

	if(buffer)
		delete buffer;
}

//Called to set up a connection to the nodes
int RGLInterface::initialize(char *address)
{
	WSADATA wsaData;
	int starterr = WSAStartup(MAKEWORD(2,2), &wsaData);

	//Error checking
    if(starterr != 0) {
        printf("Error %d occurred!\n",  WSAGetLastError());
        WSACleanup();
        return -1;
    }

	//Iterate through each pipe and establish connection
	for(int i = 0; i < num_nodes; i++) {
		if(pipes[i]->connectPipe(address) < 0) {
			printf("A pipe could not connect. Exiting.\n");
			WSACleanup();
			return -1;
		}
	}

	//Create the command buffer
	buffer = new char[BUFFER_SIZE];

	return 0;
}

//Called to close all connections
void RGLInterface::cleanUp()
{
	//Iterate through each pipe and close connection
	for(int i = 0; i < num_nodes; i++) {
		pipes[i]->closePipe();
	}
}

//Pushes a command to the buffer
void RGLInterface::pushCommand(int id)
{
	memcpy(&buffer[buffer_pointer], &id, sizeof(int));
	buffer_pointer += sizeof(int);
}

//Pushes a GLFloat value to the buffer
void RGLInterface::pushGLfloat(GLfloat value)
{
	memcpy(&buffer[buffer_pointer], &value, sizeof(GLfloat));
	buffer_pointer += sizeof(GLfloat);
}

//Pushes a GLBitfield value to the buffer
void RGLInterface::pushGLbitfield(GLbitfield value)
{
	memcpy(&buffer[buffer_pointer], &value, sizeof(GLbitfield));
	buffer_pointer += sizeof(GLbitfield);
}

//Pushes a GLenum value to the buffer
void RGLInterface::pushGLenum(GLenum value)
{
	memcpy(&buffer[buffer_pointer], &value, sizeof(GLenum));
	buffer_pointer += sizeof(GLenum);
}

//Sends data in the buffer over all pipes
void RGLInterface::sendCommand()
{
	//Iterate through each pipe and send the command
	for(int i = 0; i < num_nodes; i++) {
		if(pipes[i]->sendCommand(buffer, buffer_pointer) < 0)
			printf(" on node %d\n", i);
	}

	//Reset pointer
	buffer_pointer = 0;
}

//Sends a syncronization packet telling the nodes to swap buffers
void RGLInterface::sendSync()
{
	pushCommand(0);
	sendCommand();
}

//------------------------------------------------------------------------------
//Implementations of OpenGL functions
//------------------------------------------------------------------------------
//1: glClearColor — specify clear values for the color buffers
void RGLInterface::glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	pushCommand(1);
	pushGLfloat(red);
	pushGLfloat(green);
	pushGLfloat(blue);
	pushGLfloat(alpha);
	sendCommand();
}

//2: glClear — clear buffers to preset values
void RGLInterface::glClear(GLbitfield mask)
{
	pushCommand(2);
	pushGLbitfield(mask);
	sendCommand();
}

//3: glLoadIdentity — replace the current matrix with the identity matrix
void RGLInterface::glLoadIdentity()
{
	pushCommand(3);
	sendCommand();
}

//4: glTranslatef — multiply the current matrix by a translation matrix
void RGLInterface::glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	pushCommand(4);
	pushGLfloat(x);
	pushGLfloat(y);
	pushGLfloat(z);
	sendCommand();
}

//5: glBegin — delimit the vertices of a primitive or a group of like primitives
void RGLInterface::glBegin(GLenum mode)
{
	pushCommand(5);
	pushGLenum(mode);
	sendCommand();
}

//6: glEnd — delimit the vertices of a primitive or a group of like primitives
void RGLInterface::glEnd()
{
	pushCommand(6);
	sendCommand();
}

//7: glVertex3f — Specifies a vertex
void RGLInterface::glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	pushCommand(7);
	pushGLfloat(x);
	pushGLfloat(y);
	pushGLfloat(z);
	sendCommand();
}
