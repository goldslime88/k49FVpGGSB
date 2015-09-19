#include "sender.h"

void init_sender(Sender * sender, int id)
{
    //TODO: You should fill in this function as necessary
	pthread_cond_init(&sender->buffer_cv, NULL);
	pthread_mutex_init(&sender->buffer_mutex, NULL);
	sender->send_id = id;
    sender->input_cmdlist_head = NULL;
    sender->input_framelist_head = NULL;
    sender->seqNum = 0;
    sender->LFS = -1;
    sender->LAR = -1;
    sender->SWS = 8;
    int i;
    for(i = 0; i < 8; i++){
        sender->isCached_Frame[i] = 0;
        sender->cached_FrameArray[i] = NULL;
        sender->seqQue[i] = -1;
        gettimeofday(&(sender->lastSendTime_Frame[i]),NULL);
        
    }

}

struct timeval * sender_get_next_expiring_timeval(Sender * sender)
{
    //TODO: You should fill in this function so that it returns the next timeout that should occur
    // struct timeval    curr_timeval;
    // gettimeofday(&curr_timeval,NULL);
    // int i,minIndex=-1;
    // long timeDiff, min=1000000000;
    
    struct timeval * timeDiff = NULL;
    int i;
    for(i = 0; i < 8; i++){
        if(sender->isCached_Frame[i] == 1){
            if(timeDiff == NULL){
                timeDiff = &(sender->lastSendTime_Frame[i]);
            }
            else{
                long dif = timeval_usecdiff(timeDiff, &(sender->lastSendTime_Frame[i]));
                if(dif > 0){
                    timeDiff = &(sender->lastSendTime_Frame[i]);
                }
            }
        }
    }
        
    return timeDiff;

}


void handle_incoming_acks(Sender * sender,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming ACKs
    //    1) Dequeue the ACK from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this sender
    //    5) Do sliding window protocol for sender/receiver pair 
    int incoming_msgs_length = ll_get_length(sender->input_framelist_head);
    // fprintf(stderr, "in handle_incoming_acks\n");

    while (incoming_msgs_length > 0)
    {
        //Pop a node off the front of the link list and update the count
        LLnode * ll_inmsg_node = ll_pop_node(&sender->input_framelist_head);
        incoming_msgs_length = ll_get_length(sender->input_framelist_head);
        //DUMMY CODE: Print the raw_char_buf
        //NOTE: You should not blindly print messages!
        //      Ask yourself: Is this message really for me?
        //                    Is this message corrupted?
        //                    Is this an old, retransmitted message?           
        char * raw_char_buf = (char *) ll_inmsg_node->value;
        Frame * inframe = convert_char_to_frame(raw_char_buf);
        uint8_t isCorrect = (uint8_t)crc8Caculate(raw_char_buf, 64);

        //Free raw_char_buf
        free(raw_char_buf);
        uint16_t src;
        uint16_t dst;
        uint8_t seqNum;    
        src = (uint16_t)(inframe->send_id[0]) * 256 +  (uint16_t)(inframe->send_id[1]);
        dst = (uint16_t)(inframe->recv_id[0]) * 256 +  (uint16_t)(inframe->recv_id[1]);
        seqNum = (uint8_t)(inframe->seqNum[0]);
        int i;
        if(isCorrect != 0){
            fprintf(stderr, "ACK of %d is corrupted.\n",seqNum );
        }
        if(dst == sender->send_id && isCorrect == 0){
            for(i = 0; i < 8; i++){
                if(sender->seqQue[i] == seqNum){
                    sender->isCached_Frame[i] = 0;
                    sender->LAR++;
                    fprintf(stderr, "ACK of %d received.\n", seqNum);
                    fprintf(stderr, "LAR is %d, LFS is %d \n", sender->LAR, sender->LFS);
                    break;
                }
            }           
        }
    }
}


void handle_input_cmds(Sender * sender,
                       LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling input cmd
    //    1) Dequeue the Cmd from sender->input_cmdlist_head
    //    2) Convert to Frame
    //    3) Set up the frame according to the sliding window protocol
    //    4) Compute CRC and add CRC to Frame
    // fprintf(stderr, "in handle_input_cmds\n");


    int input_cmd_length = ll_get_length(sender->input_cmdlist_head);

            
    //Recheck the command queue length to see if stdin_thread dumped a command on us
    input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    while (input_cmd_length > 0)
    {
        //Pop a node off and update the input_cmd_length
        int LfsMinusLar;
        if(sender->LFS - sender->LAR < 0){
            LfsMinusLar = 256 + sender->LFS - sender->LAR;

        }
        else{
            LfsMinusLar = sender->LFS - sender->LAR;
        }
        if(LfsMinusLar < sender->SWS){

            LLnode * ll_input_cmd_node = ll_pop_node(&sender->input_cmdlist_head);
            input_cmd_length = ll_get_length(sender->input_cmdlist_head);
            //Cast to Cmd type and free up the memory for the node
            Cmd * outgoing_cmd = (Cmd *) ll_input_cmd_node->value;
            free(ll_input_cmd_node);
                

            //DUMMY CODE: Add the raw char buf to the outgoing_frames list
            //NOTE: You should not blindly send this message out!
            //      Ask yourself: Is this message actually going to the right receiver (recall that default behavior of send is to broadcast to all receivers)?
            //                    Does the receiver have enough space in in it's input queue to handle this message?
            //                    Were the previous messages sent to this receiver ACTUALLY delivered to the receiver?
            int msg_length = strlen(outgoing_cmd->message);
            Frame * outgoing_frame = (Frame *) malloc (sizeof(Frame));
            if (msg_length > FRAME_PAYLOAD_SIZE-1)
            {
                //Do something about messages that exceed the frame size
                char * temp_data = (char*) malloc(sizeof(char) * FRAME_PAYLOAD_SIZE);
                
                memset(temp_data,0,FRAME_PAYLOAD_SIZE);
                int i = 0;
                
                while(i < FRAME_PAYLOAD_SIZE-1){
                    temp_data[i] = (outgoing_cmd->message)[i];
                    i++;
                }

                outgoing_cmd->message = outgoing_cmd->message + FRAME_PAYLOAD_SIZE-1;
                
                
                ll_append_node(&sender->input_cmdlist_head,
                               (void *) outgoing_cmd);
                strcpy(outgoing_frame->data, temp_data);
                outgoing_frame->recv_id[0] = (char)((outgoing_cmd->dst_id>>8) & 0xFF);
                outgoing_frame->recv_id[1] = (char)((outgoing_cmd->dst_id) & 0xFF);
                outgoing_frame->send_id[0] = (char)((outgoing_cmd->src_id>>8) & 0xFF);
                outgoing_frame->send_id[1] = (char)((outgoing_cmd->src_id) & 0xFF);
                outgoing_frame->seqNum[0] = (char)(sender->seqNum);                            
                outgoing_frame->inAddition[1] = (char)(1 & 0xFF);
                

            }

            //This is probably ONLY one step you want
            else{

                
                strcpy(outgoing_frame->data, outgoing_cmd->message);
                outgoing_frame->recv_id[0] = (char)((outgoing_cmd->dst_id>>8) & 0xFF);
                outgoing_frame->recv_id[1] = (char)((outgoing_cmd->dst_id) & 0xFF);
                outgoing_frame->send_id[0] = (char)((outgoing_cmd->src_id>>8) & 0xFF);
                outgoing_frame->send_id[1] = (char)((outgoing_cmd->src_id) & 0xFF);
                outgoing_frame->seqNum[0] = (char)(sender->seqNum);
                outgoing_frame->inAddition[1] = (char)(0 & 0xFF);
            
            }

            //At this point, we don't need the outgoing_cmd
            
            //break
            //Convert the message to the outgoing_charbuf
            char * calculating_charbuf = convert_frame_to_char(outgoing_frame);
            uint8_t crc = (uint8_t)crc8Caculate(calculating_charbuf, 63);
            outgoing_frame->crc[0] = (char)(crc);

            char * outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            int i;
            for(i=0;i<8;i++){
                if(sender->isCached_Frame[i] == 0){
                    sender->isCached_Frame[i] = 1;
                    (sender->cached_FrameArray)[i] = (char*)malloc(sizeof(char)*MAX_FRAME_SIZE);
                    memset(sender->cached_FrameArray[i], 0, 64);
                    memcpy(sender->cached_FrameArray[i],outgoing_charbuf,64);
                    sender->seqQue[i] = sender->seqNum;
                    struct timeval curr;
                    gettimeofday(&curr, NULL);
                    long usTime = curr.tv_usec+100000;
                    if(usTime>=1000000){
                        (sender->lastSendTime_Frame[i]).tv_sec = curr.tv_sec+1;
                        (sender->lastSendTime_Frame[i]).tv_usec = curr.tv_usec-900000;
                    }
                    else{
                        (sender->lastSendTime_Frame[i]).tv_sec = curr.tv_sec;
                        (sender->lastSendTime_Frame[i]).tv_usec = curr.tv_usec+100000;
                    }
                    
                    break;
                }
                           
            }
            fprintf(stderr, "%d is sent.\n", sender->seqNum);
            if(sender->seqNum == 255){
                sender->seqNum = 0;
            }
            else{
                sender->seqNum++;
            }
            sender->LFS++;

            fprintf(stderr, "LAR is %d, LFS is %d \n", sender->LAR, sender->LFS);
            ll_append_node(outgoing_frames_head_ptr,
                           outgoing_charbuf);
            free(outgoing_frame);
            free(calculating_charbuf);
                    
        }
        else{
            break;
        }
    } 
    
  
}


void handle_timedout_frames(Sender * sender,
                            LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling timed out datagrams
    //    1) Iterate through the sliding window protocol information you maintain for each receiver
    //    2) Locate frames that are timed out and add them to the outgoing frames
    //    3) Update the next timeout field on the outgoing frames
    
    int i;
    long timeDiff;
    for(i = 0; i < 8; i++){
        if(sender->isCached_Frame[i] == 1 && sender->cached_FrameArray[i] != NULL){
            struct timeval    curr_timeval;
            gettimeofday(&curr_timeval,NULL);
            timeDiff = timeval_usecdiff(&(sender->lastSendTime_Frame[i]), &curr_timeval);
            if(timeDiff >=0){

                char *tempBuf =  (char *) malloc(sizeof(char)*MAX_FRAME_SIZE);
                memset(tempBuf,0,MAX_FRAME_SIZE);
                memcpy(tempBuf,sender->cached_FrameArray[i], MAX_FRAME_SIZE);

                ll_append_node(outgoing_frames_head_ptr,
                               tempBuf);
                gettimeofday(&curr_timeval,NULL);
                long usTime = curr_timeval.tv_usec+100000;
                if(usTime>=1000000){
                    (sender->lastSendTime_Frame[i]).tv_sec = curr_timeval.tv_sec+1;
                    (sender->lastSendTime_Frame[i]).tv_usec = curr_timeval.tv_usec-900000;
                }
                else{
                    (sender->lastSendTime_Frame[i]).tv_sec = curr_timeval.tv_sec;
                    (sender->lastSendTime_Frame[i]).tv_usec = curr_timeval.tv_usec+100000;
                }
                fprintf(stderr, "in %d us, %d resend.\n", curr_timeval.tv_usec, sender->seqQue[i]);

            }
        }
    }

}


void * run_sender(void * input_sender)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval; 
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Sender * sender = (Sender *) input_sender;    
    LLnode * outgoing_frames_head;
    struct timeval * expiring_timeval;
    long sleep_usec_time, sleep_sec_time;
    
    //This incomplete sender thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up
    //2. Grab the mutex protecting the input_cmd/inframe queues
    //3. Dequeues messages from the input queue and adds them to the outgoing_frames list
    //4. Releases the lock
    //5. Sends out the messages


    while(1)
    {    
        outgoing_frames_head = NULL;

        //Get the current time
        gettimeofday(&curr_timeval, 
                     NULL);

        //time_spec is a data structure used to specify when the thread should wake up
        //The time is specified as an ABSOLUTE (meaning, conceptually, you specify 9/23/2010 @ 1pm, wakeup)
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;

        //Check for the next event we should handle
        expiring_timeval = sender_get_next_expiring_timeval(sender);

        //Perform full on timeout
        if (expiring_timeval == NULL)
        {
            time_spec.tv_sec += WAIT_SEC_TIME;
            time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        }
        else
        {
            //Take the difference between the next event and the current time
            sleep_usec_time = timeval_usecdiff(&curr_timeval,
                                               expiring_timeval);

            //Sleep if the difference is positive
            if (sleep_usec_time > 0)
            {
                sleep_sec_time = sleep_usec_time/1000000;
                sleep_usec_time = sleep_usec_time % 1000000;   
                time_spec.tv_sec += sleep_sec_time;
                time_spec.tv_nsec += sleep_usec_time*1000;
            }   
        }

        //Check to make sure we didn't "overflow" the nanosecond field
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        
        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames or input commands should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&sender->buffer_mutex);

        //Check whether anything has arrived
        int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
        int inframe_queue_length = ll_get_length(sender->input_framelist_head);
        
        //Nothing (cmd nor incoming frame) has arrived, so do a timed wait on the sender's condition variable (releases lock)
        //A signal on the condition variable will wakeup the thread and reaquire the lock
        if (input_cmd_length == 0 &&
            inframe_queue_length == 0)
        {
            
            pthread_cond_timedwait(&sender->buffer_cv, 
                                   &sender->buffer_mutex,
                                   &time_spec);
        }
        //Implement this
        handle_incoming_acks(sender,
                             &outgoing_frames_head);

        //Implement this
        handle_input_cmds(sender,
                          &outgoing_frames_head);

        pthread_mutex_unlock(&sender->buffer_mutex);


        //Implement this
        handle_timedout_frames(sender,
                               &outgoing_frames_head);

        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *)  ll_outframe_node->value;
            
            //Don't worry about freeing the char_buf, the following function does that
            send_msg_to_receivers(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }

    pthread_exit(NULL);
    return 0;
}
