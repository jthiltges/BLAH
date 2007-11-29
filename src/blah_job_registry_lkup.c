/*
 *  File :     blah_job_registry_lkup.c
 *
 *  Author :   Francesco Prelz ($Author: fprelz $)
 *  e-mail :   "francesco.prelz@mi.infn.it"
 *
 *  Revision history :
 *  16-Nov-2007 Original release
 *
 *  Description:
 *   Executable to look up for an entry in the BLAH job registry.
 *
 *  Copyright (c) 2007 Istituto Nazionale di Fisica Nucleare (INFN).
 *   All rights reserved.
 *   See http://grid.infn.it/grid/license.html for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/utsname.h> /* For uname */
#include "job_registry.h"
#include "config.h"

int
main(int argc, char *argv[])
{
  int idc;
  char *registry_file=NULL;
  int need_to_free_registry_file = FALSE;
  const char *default_registry_file = "blah_job_registry.bjr";
  char *my_home;
  job_registry_index_mode mode=BY_BLAH_ID;
  char *id;
  job_registry_entry *ren;
  char *cad;
  config_handle *cha;
  config_entry *rge,*anpe,*anhe;
  job_registry_handle *rha;
  int opt_worker_node = FALSE;
  int opt_get_port = FALSE;
  char *anhname;
  struct utsname ruts;
 
  if (argc < 2)
   {
    fprintf(stderr,"Usage: %s [-w (get worker node)] [-n (get parser host:port)] [-b (look up for batch IDs)] <id>\n",argv[0]);
    return 1;
   }

  /* Look up for command line switches */
  for (idc = 1; idc < argc; idc++)
   {
    if (argv[idc][0] != '-') break;
    switch (argv[idc][1])
     {
      case 'b':
        mode = BY_BATCH_ID;
        break;
      case 'n':
        opt_get_port = TRUE;
        break;
      case 'w':
        opt_worker_node = TRUE;
        break;
     }
   }

  id = argv[idc];

  cha = config_read(NULL); /* Read config from default locations. */
  if (cha != NULL)
   {
    rge = config_get("job_registry",cha);
    if (rge != NULL) registry_file = rge->value;
   }

  if (opt_get_port)
   {
    if (cha != NULL)
     {
      anpe = config_get("async_notification_port", cha);
      anhe = config_get("async_notification_host", cha);
     }
    if (cha == NULL || anpe == NULL)
     {
      fprintf(stderr,"%s: Cannot access value of async_notification_port in BLAH config.\n",argv[0]);
      if (cha != NULL) config_free(cha);
      return 1;
     }
    if (anhe == NULL)
     {
      if (uname(&ruts) < 0)
       {
        fprintf(stderr,"%s: Cannot access uname information. Please add async_notification_host in BLAH config.\n",argv[0]);
        config_free(cha);
        return 1;
       }
      anhname = ruts.nodename;
     }
    else anhname = anhe->value;
    printf("%s:%s\n",anhname,anpe->value);
    config_free(cha);
    return 0;
   }

  if (registry_file == NULL) registry_file = getenv("BLAH_JOB_REGISTRY_FILE");
  if (registry_file == NULL)
   {
    my_home = getenv("HOME");
    if (my_home == NULL) my_home = ".";
    registry_file = (char *)malloc(strlen(default_registry_file)+strlen(my_home)+2);
    if (registry_file != NULL) 
     {
      sprintf(registry_file,"%s/%s",my_home,default_registry_file);
      need_to_free_registry_file = TRUE;
     }
    else 
     {
      if (cha != NULL) config_free(cha);
      return 1;
     }
   }

  rha=job_registry_init(registry_file, mode);

  if (rha == NULL)
   {
    fprintf(stderr,"%s: error initialising job registry: ",argv[0]);
    perror("");
    if (cha != NULL) config_free(cha);
    if (need_to_free_registry_file) free(registry_file);
    return 2;
   }

  /* Filename is stored in job registry handle. - Don't need these anymore */
  if (cha != NULL) config_free(cha);
  if (need_to_free_registry_file) free(registry_file);

  if ((ren=job_registry_get(rha, id)) == NULL)
   {
    fprintf(stderr,"%s: Entry <%s> not found: ",argv[0],id);
    perror("");
    job_registry_destroy(rha);
    return 1;
   } 

  if (!opt_worker_node)
   {
    /* Worker node not needed. Truncate it to 0. */
    ren->wn_addr[0]='\000';
   }

  cad = job_registry_entry_as_classad(ren);
  if (cad != NULL) printf("%s\n",cad);
  else 
   {
    fprintf(stderr,"%s: Out of memory.\n",argv[0]);
    free(ren);
    job_registry_destroy(rha);
    return 1;
   }

  free(cad);
  free(ren);
  job_registry_destroy(rha);
  return 0;
}
