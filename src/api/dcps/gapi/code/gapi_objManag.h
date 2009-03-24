#ifndef GAPI_OBJMANAG_H
#define GAPI_OBJMANAG_H

#include "gapi.h"

void *
gapi__header (
    void *object);

void
gapi_domainParticipantQos_free (
    void *object);

void
gapi_topicQos_free (
    void *object);

void
gapi_dataWriterQos_free (
    void *object);

void
gapi_publisherQos_free (
    void *object);

void
gapi_dataReaderQos_free (
    void *object);

void
gapi_dataReaderViewQos_free (
    void *object);

void
gapi_subscriberQos_free (
    void *object);

gapi_dataSampleSeq *
gapi_dataSampleSeq__alloc (
    void);

gapi_dataSample *
gapi_dataSampleSeq_allocbuf (
    gapi_unsigned_long len);

#endif