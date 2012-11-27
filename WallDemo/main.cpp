/*----------------------------------------------------------------------------*\
|Entry point for a render node. Parses command line and initializes GLNode     |
|                                                                              |
|Stewart Hall                                                                  |
|11/26/2012                                                                    |
\*----------------------------------------------------------------------------*/

#include "GLNode.h"

int main(int argc, char *argv[])
{
	char *configFile;
	char *nodeId;

	//Parse arguments
	for(int i = 0; i < argc; i++) {
		if(argv[i][0] == '-' && argv[i][1] == 'c') {
			//Config file argument
			i++;
			configFile = argv[i];
		} else if(argv[i][0] == '-' && argv[i][1] == 'n') {
			//Data file argument
			i++;
			nodeId = argv[i];
		}
	}

	//Initialize a GLNode
	GLNode *gln = new GLNode(configFile, nodeId);
	gln->printStatus();

	//Have the node start listening
	gln->startListening();

	return 0;
}
