/*
 *  Copyright(c), 2002 The GHC Team.
 */

#include "Rts.h"
#include "HsNet.h"

/*
 * sendmsg() and recvmsg() wrappers for transmitting
 * ancillary socket data.
 *
 * Doesn't provide the full generality of either, specifically:
 *
 *  - no support for scattered read/writes.
 *  - only possible to send one ancillary chunk of data at a time.
 *
 */

StgInt
sendAncillary(int sock,
	      int level,
	      int type,
	      int flags,
	      void* data,
	      int len)
{
  struct msghdr msg = {0};
  struct cmsghdr *cmsg;
  char ancBuffer[CMSG_SPACE(len)];
  char* dPtr;
  char  buf[2];
  struct iovec iov[1];
  
  buf[0] = 'X'; buf[1] = '\0';
  iov[0].iov_base = buf;
  iov[0].iov_len  = 2;

  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = ancBuffer;
  msg.msg_controllen = sizeof(ancBuffer);

  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = level;
  cmsg->cmsg_type = type;
  cmsg->cmsg_len = CMSG_LEN(len);
  dPtr = (char*)CMSG_DATA(cmsg);
  
  memcpy(dPtr, data, len);
  msg.msg_controllen = cmsg->cmsg_len;
  
  return sendmsg(sock,&msg,flags);
}

StgInt
recvAncillary(int  sock,
	      int* pLevel,
	      int* pType,
	      int  flags,
	      void** pData,
	      int* pLen)
{
  struct msghdr msg = {0};
  struct cmsghdr *cmsg = NULL;
  struct cmsghdr *cptr;
  struct iovec iov[1];
  char  duffBuf[10];
  
  int rc;
  
  cmsg = (struct cmsghdr*)malloc(CMSG_SPACE(*pLen));
  if (cmsg==NULL) {
    return -1;
  }
  
  iov[0].iov_base = duffBuf;
  iov[0].iov_len  = sizeof(duffBuf);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsg;
  msg.msg_controllen = CMSG_LEN(*pLen);

  if ((rc = recvmsg(sock,&msg,flags)) < 0) {
    return rc;
  }
  
  cptr = (struct cmsghdr*)CMSG_FIRSTHDR(&msg);

  *pLevel = cptr->cmsg_level;
  *pType  = cptr->cmsg_type;
  /* The length of the data portion only */
  *pLen   = cptr->cmsg_len - sizeof(struct cmsghdr);
  *pData  = CMSG_DATA(cptr);

  return rc;
}