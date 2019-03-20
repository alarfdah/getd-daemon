* Checked the message type provided with the header.
* Checked the received bytes against the message length in the header.
* Checked the message length vs sizeof(MessageType).
* Checked the existence of the NULL character at the end of each buffer.
* Checked the length of buffers to make sure they don't overflow.
* Checked the expected message types.
* Checked the session ID against received session IDs.
* Checked the time it takes to receive and send messages and timed out on 7 secondes.