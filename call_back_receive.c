 case LWS_CALLBACK_RECEIVE: { // the funny part
   // create a buffer to hold our response
   // it has to have some pre and post padding. You don't need to care
   // what comes there, libwebsockets will do everything for you. For more info see
   // http://git.warmcat.com/cgi-bin/cgit/libwebsockets/tree/lib/libwebsockets.h#n597
   unsigned char *buf = (unsigned char*) malloc(LWS_SEND_BUFFER_PRE_PADDING + len +
						LWS_SEND_BUFFER_POST_PADDING);

   int i;

   // pointer to `void *in` holds the incomming request
   // we're just going to put it in reverse order and put it in `buf` with
   // correct offset. `len` holds length of the request.
   for (i=0; i < len; i++) {
     buf[LWS_SEND_BUFFER_PRE_PADDING + (len - 1) - i ] = ((char *) in)[i];
   }

   // log what we recieved and what we're going to send as a response.
   // that disco syntax `%.*s` is used to print just a part of our buffer
   // http://stackoverflow.com/questions/5189071/print-part-of-char-array
   printf("received data: %s, replying: %.*s\n", (char *) in, (int) len,
	  buf + LWS_SEND_BUFFER_PRE_PADDING);

   // send response
   // just notice that we have to tell where exactly our response starts. That's
   // why there's `buf[LWS_SEND_BUFFER_PRE_PADDING]` and how long it is.
   // we know that our response has the same length as request because
   // it's the same message in reverse order.
   libwebsocket_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], len, LWS_WRITE_TEXT);

   // release memory back into the wild
   free(buf);
   break;
