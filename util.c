#include "util.h"

//Linked list functions
int ll_get_length(LLnode * head)
{
    LLnode * tmp;
    int count = 1;
    if (head == NULL)
        return 0;
    else
    {
        tmp = head->next;
        while (tmp != head)
        {
            count++;
            tmp = tmp->next;
        }
        return count;
    }
}

void ll_append_node(LLnode ** head_ptr, 
                    void * value)
{
    LLnode * prev_last_node;
    LLnode * new_node;
    LLnode * head;

    if (head_ptr == NULL)
    {
        return;
    }
    
    //Init the value pntr
    head = (*head_ptr);
    new_node = (LLnode *) malloc(sizeof(LLnode));
    new_node->value = value;

    //The list is empty, no node is currently present
    if (head == NULL)
    {
        (*head_ptr) = new_node;
        new_node->prev = new_node;
        new_node->next = new_node;
    }
    else
    {
        //Node exists by itself
        prev_last_node = head->prev;
        head->prev = new_node;
        prev_last_node->next = new_node;
        new_node->next = head;
        new_node->prev = prev_last_node;
    }
}


LLnode * ll_pop_node(LLnode ** head_ptr)
{
    LLnode * last_node;
    LLnode * new_head;
    LLnode * prev_head;

    prev_head = (*head_ptr);
    if (prev_head == NULL)
    {
        return NULL;
    }
    last_node = prev_head->prev;
    new_head = prev_head->next;

    //We are about to set the head ptr to nothing because there is only one thing in list
    if (last_node == prev_head)
    {
        (*head_ptr) = NULL;
        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    }
    else
    {
        (*head_ptr) = new_head;
        last_node->next = new_head;
        new_head->prev = last_node;

        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    }
}

void ll_destroy_node(LLnode * node)
{
    if (node->type == llt_string)
    {
        free((char *) node->value);
    }
    free(node);
}

//Compute the difference in usec for two timeval objects
long timeval_usecdiff(struct timeval *start_time, 
                      struct timeval *finish_time)
{
  long usec;
  usec=(finish_time->tv_sec - start_time->tv_sec)*1000000;
  usec+=(finish_time->tv_usec- start_time->tv_usec);
  return usec;
}


//Print out messages entered by the user
void print_cmd(Cmd * cmd)
{
    fprintf(stderr, "src=%d, dst=%d, message=%s\n", 
           cmd->src_id,
           cmd->dst_id,
           cmd->message);
}


char * convert_frame_to_char(Frame * frame)
{
    //TODO: You should implement this as necessary
    char * char_buffer = (char *) malloc(MAX_FRAME_SIZE);
    //fprintf(stderr, "begin convert_frame_to_char\n");
    memset(char_buffer,
           0,
           MAX_FRAME_SIZE);
    memcpy(char_buffer, 
           frame->send_id,
           2);
    memcpy(char_buffer+2, 
           frame->recv_id,
           2);
    memcpy(char_buffer+4, 
           frame->seqNum,
           1);
    memcpy(char_buffer+5, 
           frame->inAddition,
           2);
    memcpy(char_buffer+7, 
           frame->data,
           56);
    memcpy(char_buffer+63, 
           frame->crc,
           1);



    return char_buffer;
}


Frame * convert_char_to_frame(char * char_buf)
{
    //TODO: You should implement this as necessary
    Frame * frame = (Frame *) malloc(sizeof(Frame));
    //fprintf(stderr, "begin convert_char_to_frame\n");
    
    memset(frame->send_id,
           0,
           sizeof(char)*sizeof(frame->send_id));
    memcpy(frame->send_id, 
           char_buf,
           sizeof(char)*sizeof(frame->send_id));
    memset(frame->recv_id,
           0,
           sizeof(char)*sizeof(frame->recv_id));
    memcpy(frame->recv_id, 
           char_buf+2,
           sizeof(char)*sizeof(frame->recv_id));
    memset(frame->seqNum,
           0,
           sizeof(char)*sizeof(frame->seqNum));
    memcpy(frame->seqNum, 
           char_buf+4,
           sizeof(char)*sizeof(frame->seqNum));
    memset(frame->inAddition,
           0,
           sizeof(char)*sizeof(frame->inAddition));
    memcpy(frame->inAddition, 
           char_buf+5,
           sizeof(char)*sizeof(frame->inAddition));
    memset(frame->data,
           0,
           sizeof(char)*sizeof(frame->data));
    memcpy(frame->data, 
           char_buf+7,
           sizeof(char)*sizeof(frame->data));
    memset(frame->crc,
           0,
           sizeof(char)*sizeof(frame->crc));
    memcpy(frame->crc, 
           char_buf+63,
           sizeof(char)*sizeof(frame->crc));


    return frame;
}

uint8_t crc8Caculate(char* pendingData, int len){
  uint8_t crc = 0xFF;
  int i,j;
  for(i = 0;i < len; i++){
    crc ^= (uint8_t)pendingData[i];
    for (j = 0; j < 8; j++)
    {
    if (crc & 0x01)
        crc = (crc >> 1) ^ 0x9c;
    else
        crc = (crc >> 1);
    }
  }

  return crc;
}



