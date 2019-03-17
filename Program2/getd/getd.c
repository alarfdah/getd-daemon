#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include "../util/message.h"

/**
  * Generating random string.
  * Reference: https://stackoverflow.com/questions/15767691/whats-the-c-library-function-to-generate-random-string
  */
void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}

void check_file_size(FILE **ptr, long *size) {
	// Get file size
	assert(fseek((*ptr), 0, SEEK_END) != -1 && "fseek failed!");
	(*size) = ftell((*ptr));
	assert((*size) > 0 && "file is empty!");

	// Reset file_pointer to beginning
	assert(fseek((*ptr), 0, SEEK_SET) != -1 && "fseek failed!");
}

void check_file_permissions(char *path, char *name) {
	struct stat file_stat;

	// If path does not exist
	if (lstat(path, &file_stat) == -1) {
		printf("check_file_permissions lstat failed, %s does not exist!\n", path);
		exit(-1);
	}

	// Check permissions
	if (!(file_stat.st_mode & S_IRUSR)) {
      printf("%s does not have read permissions!\n", name);
      exit(-1);
  } else {
  }
}

void check_if_user_owns_file(char *path, char *name, unsigned int name_len) {
  struct passwd *pw;
  struct stat file_stat;
  char *file_owner;
  int file_owner_len;
  int length;

	// fstat the file
	assert(lstat(path, &file_stat) != -1 && "check_if_owner_owns_file: lstat in failed!");

	// Get file's associated user
	pw = getpwuid(file_stat.st_uid);
	assert(pw != NULL && "passwd struct (st_uid) is NULL!");

	// Store file's owner
	file_owner_len = strlen(pw->pw_name);
	assert(file_owner_len > 0 && "length is not > 0!");
	file_owner = malloc(file_owner_len + 1);
	assert(file_owner != NULL && "file_owner malloc is NULL!");
	assert(strncpy(file_owner, pw->pw_name, file_owner_len) != NULL && "strncpy on file_owner is NULL!");


	/* UNCOMMENT TO TEST VALUES *
		printf("Owner  name: %s\n", suid_name);
		printf("File  owner: %s\n", file_owner);
	*/

	// Find the lesser length
	if (name_len < file_owner_len)
		length = name_len;
	else
		length = file_owner_len;

	// Compare owner's name with the file owner's name
	if (strncmp(name, file_owner, length) != 0) {
		printf("%s does not own %s\n", name, path);
		exit(-1);
	}
}

void check_received_vs_expected(int prev, int message_type) {
  // Expecting type 0
  if ((prev == -1 || prev == 2) && message_type != 0) {
    printf("Wrong Message Type: Expected 0, Recieved %d\n", message_type);
    exit(-1);
    // Expecting 3
  } else if (prev == 0 && message_type != 3) {
    printf("Wrong Message Type: Expected 3, Recieved %d\n", message_type);
    exit(-1);
    // Expecting 6
  } else if (prev == 3 && message_type != 6) {
    printf("Wrong Message Type: Expected 6, Recieved %d\n", message_type);
    exit(-1);
    // Expecting 6 again
  } else if (prev == 6 && message_type != 6) {
    printf("Wrong Message Type: Expected 6, Recieved %d\n", message_type);
    exit(-1);
  }
}

void read_file(FILE **ptr, char **str, long *size) {
	// Read file
	(*str) = malloc((*size) + 1);
	assert((*str) != NULL && "str malloc failed!");
	assert(fread((*str), (*size), 1, (*ptr)) != 0 && "fread failed!");
}

/**
  *
  * Type 0 handler
  * Received
  *
  */
void handle_type0(MessageType0 *type0, char **name, unsigned int *name_len) {
  // Error Checking - Message Type
  if (type0->header.messageType != 0) {
    printf("handle_type0: Message Type is not 0\n");
    printf("Message Type = %d\n", type0->header.messageType);
  }

  // Error Checking - Message Length vs Actual Size
  if (type0->header.messageLength != sizeof(MessageType0)) {
    printf("handle_type0: Incorrect MessageType0 length!\n");
    printf("type0->messageLength = %d\n", type0->header.messageLength);
    printf("MessageType0 Length = %ld\n", sizeof(MessageType0));
    exit(-1);
  }

  // Print Info
  printf("Received Message Type: %d\n", type0->header.messageType);
  printf("Received Message Length: %d\n\n", type0->header.messageLength);


  // Get username length
  (*name_len) = type0->dnLength;
  printf("name_length = %d\n", (*name_len));

  // Error Checking - Username length
  if ((*name_len) > DN_LENGTH) {
    printf("handle_type0: Name length is too large.\n");
    exit(-1);
  } else if ((*name_len) == 0) {
    printf("handle_type0: Name length is 0.\n");
    exit(-1);
  }

  // Get username + Error Checking
  (*name) = malloc((*name_len) + 1);
  assert((*name) != NULL && "handle_type0 name is NULL!");
  assert(strncpy((*name), type0->distinguishedName, (*name_len)) != NULL && "handle_type0 strncpy is NULL!");
  (*name)[(*name_len)] = '\0';

}

/**
  *
  * Type 1 handler
  * Sent
  *
  */
void handle_type1(int sock, MessageType1 *type1, char *session_id) {
  // Create type1 Struct
  type1 = malloc(sizeof(MessageType1));

  // Error Checking
  assert(type1 != NULL && "handle_type1 type1 is NULL!");

  // Initialize type1
  type1->header.messageType = 1;
  type1->header.messageLength = sizeof(MessageType1);
  type1->sidLength = SID_LENGTH;
  assert(strncpy(type1->sessionId, session_id, SID_LENGTH) && "handle_type1 strncpy failed!");
  type1->sessionId[SID_LENGTH] = '\0';

  // Print Info
  printf("Sending Message Type: %d\n", type1->header.messageType);
  printf("Sending Message Length: %d\n", type1->header.messageLength);
  printf("Session ID Length: %d\n", type1->sidLength);
  printf("Session ID: %s\n\n", type1->sessionId);

  // Send back the contents
  assert(nn_send(sock, type1, sizeof(MessageType1), 0) != -1 && "handle_type1 nn_send(type1) failed!");

}

/**
  *
  * Type 2 handler
  * Received
  *
  */
void handle_type2(MessageType2 *type2) {

  // Error Checking - Message length
  if (type2->msgLength > MAX_ERROR_MESSAGE) {
    printf("handle_type2: Error message too large!\n");
    exit(-1);
  } else if (type2->msgLength == 0) {
    printf("handle_type2: Error message is 0!\n");
    exit(-1);
  }

  // Print Info
  printf("Received Message Type: %d\n", type2->header.messageType);
  printf("Received Message Length: %d\n", type2->header.messageLength);
  printf("Error: %s\n\n", type2->errorMessage);
}

/**
  *
  * Type 3 handler
  * Received
  *
  */
void handle_type3(MessageType3 *type3, FILE **fp, char *session_id, char *name,
  unsigned int name_len, char **content, long *content_len) {
    // Local variables
    struct stat lstat_info;
    struct stat fstat_info;
    int fd;
    int path_length;

    // Error Checking - Message Type
    if (type3->header.messageType != 3) {
      printf("handle_type3: Message Type is not 3.\n");
      printf("Message Type = %d\n", type3->header.messageType);
    }

    // Error Checking - Message Length vs Actual Size
    if (type3->header.messageLength != sizeof(MessageType3)) {
      printf("handle_type3: Incorrect MessageType3 length!\n");
      printf("type3->messageLength = %d\n", type3->header.messageLength);
      printf("MessageType3 Length = %ld\n", sizeof(MessageType3));
      exit(-1);
    }

    // Error Checking - Session ID
    if (strncmp(session_id, type3->sessionId, SID_LENGTH) != 0) {
      printf("handle_type3: Session ID does not match original.\n");
      printf("Original Session ID: %s\n", session_id);
      printf("Type3 Session ID: %s\n", type3->sessionId);
      exit(-1);
    }

    // Print info
    printf("Received Message Type: %d\n", type3->header.messageType);
    printf("Received Message Length: %d\n\n", type3->header.messageLength);

    // Error Checking - Path length vs Actual path length
    path_length = strlen(type3->pathName);
    assert(path_length > 0 && "handle_type3 type3->pathName is 0.");
    if (path_length != type3->pathLength) {
      printf("handle_type3: type3->pathLength and actual path length are different.\n");
      printf("type3->pathLength = %d\n", type3->pathLength);
      printf("Actual path length = %d \n", path_length);
    }

    // Error Checking - Check for ownership
    check_if_user_owns_file(type3->pathName, name, name_len);

    // Error Checking - Check for read permission
    check_file_permissions(type3->pathName, name);

    // Error Checking - lstat for existence
    if (lstat(type3->pathName, &lstat_info) == -1) {
       printf("File is a symbolic link.\n");
       exit(-1);
    }

    // openfile
    (*fp) = fopen(type3->pathName, "r");

    // Error Checking
    assert((*fp) != NULL && "handle_type3 fopen is NULL! File does not exist.");

    // Error Checking - fstat for comparison with lstat
    fd = fileno((*fp));
    if (fstat(fd, &fstat_info) == -1) {
      printf("fstat error.\n");
    }

    // Error Checking - Compare modes for Symbolic links, iNodes for TOCTOU
    if (lstat_info.st_mode == fstat_info.st_mode && lstat_info.st_ino == fstat_info.st_ino) {
      // Get file size
      check_file_size(fp, content_len);

      // Read file
      read_file(fp, content, content_len);

    // Error
    } else {
      printf("File is symbolic or TOCTOU occured!\n");
      exit(-1);
    }

}

/**
  *
  * Type 4 handler
  * Sent
  *
  */
void handle_type4(int sock, MessageType4 *type4, char *session_id,
  char **content, long *content_len) {
    
  // Create type 4 Struct
  type4 = malloc(sizeof(MessageType4));

  // Error Checking
  assert(type4 != NULL && "handle_type4 type4 is NULL!");

  // Initialize type4
  type4->header.messageType = 4;
  type4->header.messageLength = sizeof(MessageType4);
  type4->sidLength = SID_LENGTH;
  assert(strncpy(type4->sessionId, session_id, SID_LENGTH));
  type4->sessionId[SID_LENGTH] = '\0';

  // Print Info
  printf("Sending Message Type: %d\n", type4->header.messageType);
  printf("Sending Message Length: %d\n", type4->header.messageLength);
  printf("Session ID Length: %d\n", type4->sidLength);
  printf("Session ID: %s\n\n", type4->sessionId);

  // Error Checking
  assert((*content) != NULL && "handle_type4 content is NULL!");

  // Content to send
  if ((*content_len) > MAX_CONTENT_LENGTH) {
    type4->contentLength = MAX_CONTENT_LENGTH;
    assert(strncpy(type4->contentBuffer, (*content), MAX_CONTENT_LENGTH) != NULL && "handle_type4 content_len > 4096 strncpy failed!");
    type4->contentBuffer[MAX_CONTENT_LENGTH] = '\0';
    (*content_len) -= MAX_CONTENT_LENGTH;
    (*content) += MAX_CONTENT_LENGTH;
    // printf("content length = %d\n", type4->contentLength);
    // printf("content = %s\n", type4->contentBuffer);
  } else {
    type4->contentLength = (*content_len);
    assert(strncpy(type4->contentBuffer, (*content), (*content_len)) != NULL && "handle_type4 content_len > 4096 strncpy failed!");
    type4->contentBuffer[MAX_CONTENT_LENGTH] = '\0';
    (*content_len) -= (*content_len);
    (*content) += (*content_len);
    // printf("content length = %d\n", type4->contentLength);
    // printf("content = %s\n", type4->contentBuffer);
  }

  // Send the contents
  assert(nn_send(sock, type4, sizeof(MessageType4), 0) != -1 && "handle_type4 nn_send is NULL!");
}

/**
  *
  * Type 5 handler
  * Sent
  *
  */
void handle_type5(int sock, MessageType5 *type5, char *session_id) {
  // Create type1 Struct
  type5 = malloc(sizeof(MessageType5));

  // Error Checking
  assert(type5 != NULL && "type5 is NULL!");

  // Initialize type1
  type5->header.messageType = 5;
  type5->header.messageLength = sizeof(MessageType5);
  type5->sidLength = SID_LENGTH;
  assert(strncpy(type5->sessionId, session_id, SID_LENGTH));
  type5->sessionId[SID_LENGTH] = '\0';

  // Print Info
  printf("Sending Message Type: %d\n", type5->header.messageType);
  printf("Sending Message Length: %d\n\n", type5->header.messageLength);

  // Send back the contents
  assert(nn_send(sock, type5, sizeof(MessageType5), 0) != -1 && "handle_type5 nn_send(type5) failed!");
}

/**
  *
  * Type 6 handler
  * Received
  *
  */
void handle_type6(MessageType6 *type6, char *session_id) {
  // Error Checking - Message Type
  if (type6->header.messageType != 6) {
    printf("handle_type6: Message Type is not 6.\n");
    printf("Message Type = %d\n", type6->header.messageType);
  }

  // Error Checking - Message Length vs Actual Size
  if (type6->header.messageLength != sizeof(MessageType6)) {
    printf("handle_type6: Incorrect MessageType6 length!\n");
    printf("type6->messageLength = %d\n", type6->header.messageLength);
    printf("MessageType6 Length = %ld\n", sizeof(MessageType6));
    exit(-1);
  }

  // Error Checking - Session ID
  if (strncmp(session_id, type6->sessionId, SID_LENGTH) != 0) {
    printf("handle_type6: Session ID does not match original.\n");
    printf("Original Session ID: %s\n", session_id);
    printf("Type6 Session ID: %s\n", type6->sessionId);
    exit(-1);
  }

  // Print Info
  printf("Receiving Message Type: %d\n", type6->header.messageType);
  printf("Receiving Message Length: %d\n\n", type6->header.messageLength);

}

int main(const int argc, const char **argv) {
  // Message Structs
  MessageType0 *type0 = NULL;
  MessageType1 *type1 = NULL;
  MessageType2 *type2 = NULL;
  MessageType3 *type3 = NULL;
  MessageType4 *type4 = NULL;
  MessageType5 *type5 = NULL;
  MessageType6 *type6 = NULL;

  // receive buffer
  void *buf = NULL;

  // Message Type
  int message_type;

  // User info
  char *uid_name = NULL;
  unsigned int uid_len;

  // File content
  FILE *fp;
  char *content = NULL;
  long content_len;

  // Session info
  char *session_id;

  // create a socket, setting type to REPLY
  int sock = nn_socket(AF_SP, NN_PAIR);

  // Flags
  int msg5_sent = 0;

  // Prev and Next states
  int prev = -1;

  // Error Checking
  printf("sock = %d\n", sock);
  assert(sock >= 0 && "nn_socket failed!");

  // connect to the socket + Error Checking
  assert(nn_bind(sock, IPC_ADDR) >= 0 && "nn_bind failed!");

  do {
    // receive message from socket and display + Error Checking
    assert(nn_recv(sock, &buf, NN_MSG, 0) >= 0 && "nn_recv failed!");

    // get message type
    message_type = ((Header *)buf)->messageType;

    // Error Checking
    assert(message_type != NULL && "Received message is NULL!");

    // Error Checking - Check expected vs actual message type
    check_received_vs_expected(prev, message_type);


    switch (message_type) {
      case 0:
        prev = 0;
        type0 = (MessageType0 *) buf;

        handle_type0(type0, &uid_name, &uid_len);
        session_id = calloc(SID_LENGTH + 1, sizeof(char));
        rand_str(session_id, SID_LENGTH);
        handle_type1(sock, type1, session_id);
      break;
      case 2:
        prev = 2;
        type2 = (MessageType2 *) buf;
        handle_type2(type2);
      break;
      case 3:
        prev = 3;
        type3 = (MessageType3 *) buf;
        handle_type3(type3, &fp, session_id, uid_name, uid_len, &content, &content_len);
        handle_type4(sock, type4, session_id, &content, &content_len);
      break;
      case 6:
        prev = 6;
        type6 = (MessageType6 *) buf;
        handle_type6(type6, session_id);
        if (content_len > 0) {
          handle_type4(sock, type4, session_id, &content, &content_len);
          msg5_sent = 0;
        } else if (!msg5_sent) {
          handle_type5(sock, type5, session_id);
          msg5_sent = 1;
        // Reciving 6 after sending 5
        } else {
          prev = -1;
        }
      break;
      default:
        printf("Message type %d is invalid.\n", ((Header *)buf)->messageType);
        exit(-1);
    }


  } while (1);
}
