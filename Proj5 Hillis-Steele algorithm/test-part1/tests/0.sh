#!/bin/bash

grep pthread.h ../scan.c > /dev/null

if [ "$?" -ne 0 ]; then
  # RJW: Copy output to stderr so that the script will output this message on failure.
  >&2 echo "Reason for failure: Program does not appear to use the pthreads library!"
  exit 1
fi

exit 0
