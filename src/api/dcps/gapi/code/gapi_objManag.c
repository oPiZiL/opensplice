#include "gapi.h"

#include "os_heap.h"
#include "os_abstract.h"
#include "gapi_common.h"

#define MEM_ALIGNMENT	8
#define HMM_MAGIC       (0xabcdefed)

#define ALIGN_SIZE(value) ((((value) + MEM_ALIGNMENT - 1)/MEM_ALIGNMENT)*MEM_ALIGNMENT)

#define CHECK_REF (0)

#if CHECK_REF
#define CHECK_REF_DEPTH (32)
static char* CHECK_REF_FILE = NULL;

#define UT_TRACE(msgFormat, ...) do { \
    void *tr[CHECK_REF_DEPTH];\
    char **strs;\
    size_t s,i; \
    FILE* stream; \
    \
    if(!CHECK_REF_FILE){ \
        CHECK_REF_FILE = os_malloc(16); \
        sprintf(CHECK_REF_FILE, "heap.log"); \
    } \
    s = backtrace(tr, CHECK_REF_DEPTH);\
    strs = backtrace_symbols(tr, s);\
    stream = fopen(CHECK_REF_FILE, "a");\
    fprintf(stream, msgFormat, __VA_ARGS__);              \
    for (i=0;i<s;i++) fprintf(stream, "%s\n", strs[i]);\
    fprintf(stream, "\n\n\n"); \
    free(strs);\
    fflush(stream);\
    fclose(stream);\
  } while (0)
#else
#define UT_TRACE(msgFormat, ...)
#endif

typedef void (*dealloactorType)(void *);

typedef struct {
    dealloactorType deallocator;
    gapi_unsigned_long magic;
    void *alloc_addr;
} contextHeader;

typedef struct {
    gapi_unsigned_long _maximum;
    gapi_unsigned_long _length;
    void *_buffer;
    gapi_boolean _release;
} sequenceType;

const gapi_unsigned_long CONTEXTHEADER_SIZE = ALIGN_SIZE(sizeof(contextHeader));
    
void *
gapi__malloc (
    void (*ff)(void *),
    gapi_unsigned_long hl,
    gapi_unsigned_long len
    )
{
    unsigned long totlen;
    void *header;
    void *data;
    contextHeader *context;

    totlen = ALIGN_SIZE(hl) + CONTEXTHEADER_SIZE + len;
    header = os_malloc(totlen);
    if (header) {
        memset(header, 0, totlen);
        context = (contextHeader *)((PA_ADDRCAST)header + ALIGN_SIZE(hl));
        data = (void *)((PA_ADDRCAST)context + CONTEXTHEADER_SIZE);
        context->deallocator = ff;
        context->magic = HMM_MAGIC;
        context->alloc_addr = header;
    }
    return data;
}

void
gapi__free (
    void *object
    )
{
    contextHeader *context;

    if (object != NULL) {
        context = (contextHeader *)((PA_ADDRCAST)object - CONTEXTHEADER_SIZE);
        UT_TRACE("gapi__free(%x) header %x context %x\n", (unsigned int)object,
            (unsigned int)context->alloc_addr, (unsigned int)context);
        if (context->magic == HMM_MAGIC) {
            if (context->deallocator == NULL) {
                context->magic = 0;
                os_free(context->alloc_addr);
            } else {
                context->deallocator(object);
                context->magic = 0;
                os_free(context->alloc_addr);
            }
        }
    }
}

void *
gapi__header (
    void *object)
{
    contextHeader *context;

    if (object != NULL) {
        context = (contextHeader *)((PA_ADDRCAST)object - CONTEXTHEADER_SIZE);
        if (context->magic == HMM_MAGIC) {
            return context->alloc_addr;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

void *
gapi_alloc (
    gapi_unsigned_long l
    )
{
    void *m;

    m = gapi__malloc (NULL, 0, l);
    return m;
}

void
gapi_free (
    void *a
    )
{
    gapi__free (a);
}

gapi_char *
gapi_string_alloc (
    gapi_unsigned_long len
    )
{
    return gapi__malloc (NULL, 0, len+1);
}

gapi_char *
gapi_string_dup (
    const gapi_char *src
    )
{
    gapi_char *dst;

    if (src != NULL) {
        dst = gapi_string_alloc (strlen(src));
        strcpy (dst, src);
    } else {
        dst = NULL;
    }
    return dst;
}

void 
gapi_string_clean (
    gapi_char **string
    )
{
    if ((string != NULL) && (*string != NULL)) {
        gapi_free (*string);
        *string = NULL;
    }
}

void 
gapi_string_replace (
    gapi_char *src,
    gapi_char **dst
    )
{
    gapi_free (*dst);
    if (src) {
        *dst = gapi_string_dup (src);
    } else {
        *dst = NULL;
    }
}

void 
gapi_sequence_free (
    void *sequence
    )
{
    sequenceType *seq;

    if (sequence != NULL) {
        seq = (sequenceType *)sequence;
        if (seq->_release) {
            gapi_free (seq->_buffer);
        }
    }
}

void 
gapi_sequence_clean (
    void *sequence
    )
{
    sequenceType *seq;

    if (sequence != NULL) {
        seq = (sequenceType *)sequence;
        if (seq->_release) {
            gapi_free (seq->_buffer);
        }
        seq->_buffer = NULL;
        seq->_maximum = 0;
        seq->_length = 0;
        seq->_release = FALSE;
    }
}

void *
gapi_sequence_malloc (
    void
    )
{
    return gapi__malloc (gapi_sequence_free, 0, sizeof(sequenceType));
}

void *
gapi_sequence_allocbuf (
    void (*ff)(void *),
    gapi_unsigned_long len,
    gapi_unsigned_long count
    )
{
    void *buffer;
    gapi_unsigned_long *bufcount;

    if (count > 0) {
        buffer = gapi__malloc (ff, sizeof(gapi_unsigned_long), len * count);
        bufcount = gapi__header (buffer);
        *bufcount = count;
    } else {
        buffer = NULL;
    }

    return buffer;
}
void
gapi_sequence_replacebuf (
    void *sequence,
    _bufferAllocatorType allocbuf,
    gapi_unsigned_long count
    )
{
    sequenceType *seq;

    seq = (sequenceType *)sequence;
    if (count > seq->_maximum) {
        gapi_sequence_clean(seq);
    }
    if (seq->_buffer == NULL) {
        seq->_buffer = allocbuf(count);
        seq->_maximum = count;
        seq->_length = 0;
        seq->_release = TRUE;
    }
}

void *
gapi_sequence_create (
    void (*ff)(void *),
    gapi_unsigned_long len,
    gapi_unsigned_long count
    )
{
    sequenceType *seq;

    seq = gapi_sequence_malloc ();
    if (seq) {
        seq->_buffer = gapi_sequence_allocbuf (ff, len, count);
        if (seq->_buffer) {
            seq->_maximum = count;
            seq->_length = count;
            seq->_release = TRUE;
        }
    }
    return seq;
}

gapi_fooSeq *
gapi_fooSeq__alloc (
    void
    )
{
    return (gapi_fooSeq *)gapi_sequence_malloc ();
}
gapi_foo *
gapi_fooSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_foo *)gapi_sequence_allocbuf (NULL, sizeof(gapi_foo), len);
}

gapi_instanceHandleSeq *
gapi_instanceHandleSeq__alloc (
    void
    )
{
    return (gapi_instanceHandleSeq *)gapi_sequence_malloc ();
}

gapi_instanceHandle_t *
gapi_instanceHandleSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_instanceHandle_t *)gapi_sequence_allocbuf (NULL, sizeof(gapi_instanceHandle_t), len);
}

void
gapi_stringSeq_freebuf (
    void *buffer
    )
{
    gapi_string *b = (gapi_string *)buffer;
    gapi_unsigned_long *count = (gapi_unsigned_long *)gapi__header (buffer);
    gapi_unsigned_long i;

    for (i = 0; i < *count; i++) {
        gapi_free (b[i]);
    }
}

gapi_stringSeq *
gapi_stringSeq__alloc (
    void
    )
{
    return (gapi_stringSeq *)gapi_sequence_malloc ();
}

gapi_string *
gapi_stringSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_string *)gapi_sequence_allocbuf (gapi_stringSeq_freebuf, sizeof(gapi_string *), len);
}

gapi_boolean
gapi_stringSeq_set_length (
    gapi_stringSeq    *seq,
    gapi_unsigned_long len)
{
    gapi_boolean result = TRUE;
    gapi_string *buffer = NULL;
    
    if ( seq->_maximum > 0UL ) {
        assert(seq->_buffer);
        if ( len != seq->_maximum ) {
            buffer = gapi_stringSeq_allocbuf(len);
            
            if ( buffer ) {
                if ( seq->_release ) {
                    gapi_free(seq->_buffer);
                }
                seq->_release = TRUE;
                seq->_maximum = len;
            } else {
                result = FALSE;
            }
        } else {
            buffer = seq->_buffer;
        }
    } else {
        buffer = gapi_stringSeq_allocbuf(len);
        if ( buffer ) {
            seq->_release = TRUE;
            seq->_maximum = len;
        } else {
            result = FALSE;
        }
    }

    if ( result ) {
        seq->_length = len;
        seq->_buffer = buffer;
    }
    
    return result;
}



gapi_duration_t *
gapi_duration_t__alloc (
    void
    )
{
    return (gapi_duration_t *)gapi__malloc (NULL, 0, sizeof (gapi_duration_t));
}

gapi_time_t *
gapi_time_t__alloc (
    void
    )
{
    return (gapi_time_t *)gapi__malloc (NULL, 0, sizeof (gapi_time_t));
}

gapi_qosPolicyCountSeq *
gapi_qosPolicyCountSeq__alloc (
    void
    )
{
    return (gapi_qosPolicyCountSeq *)gapi_sequence_malloc ();
}

gapi_qosPolicyCount *
gapi_qosPolicyCountSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_qosPolicyCount *)gapi_sequence_allocbuf (NULL, sizeof (gapi_qosPolicyCount), len);
}

gapi_topicSeq *
gapi_topicSeq__alloc (
    void
    )
{
    return (gapi_topicSeq *)gapi_sequence_malloc ();
}

gapi_topic *
gapi_topicSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_topic *)gapi_sequence_allocbuf (NULL, sizeof (gapi_topic), len);
}

gapi_dataReaderSeq *
gapi_dataReaderSeq__alloc (
    void
    )
{
    return (gapi_dataReaderSeq *)gapi_sequence_malloc ();
}

gapi_dataReader *
gapi_dataReaderSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_dataReader *)gapi_sequence_allocbuf (NULL, sizeof (gapi_dataReader), len);
}

gapi_dataReaderViewSeq *
gapi_dataReaderViewSeq__alloc (
    void
    )
{
    return (gapi_dataReaderViewSeq *)gapi_sequence_malloc ();
}

gapi_dataReaderView *
gapi_dataReaderViewSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_dataReaderView *)gapi_sequence_allocbuf (NULL, sizeof (gapi_dataReaderView), len);
}

struct gapi_topicListener *
gapi_topicListener__alloc (
    void
    )
{
    return (struct gapi_topicListener *)gapi__malloc (NULL, 0, sizeof (struct gapi_topicListener));
}

struct gapi_dataWriterListener *
gapi_dataWriterListener__alloc (
    void
    )
{
    return (struct gapi_dataWriterListener *)gapi__malloc (NULL, 0, sizeof (struct gapi_dataWriterListener));
}

struct gapi_publisherListener *
gapi_publisherListener__alloc (
    void
    )
{
    return (struct gapi_publisherListener *)gapi__malloc (NULL, 0, sizeof (struct gapi_publisherListener));
}

struct gapi_dataReaderListener *
gapi_dataReaderListener__alloc (
    void
    )
{
    return (struct gapi_dataReaderListener *)gapi__malloc (NULL, 0, sizeof (struct gapi_dataReaderListener));
}

struct gapi_subscriberListener *
gapi_subscriberListener__alloc (
    void
    )
{
    return (struct gapi_subscriberListener *)gapi__malloc (NULL, 0, sizeof (struct gapi_subscriberListener));
}

struct gapi_domainParticipantListener *
gapi_domainParticipantListener__alloc (
    void
    )
{
    return (struct gapi_domainParticipantListener *)gapi__malloc (NULL, 0, sizeof (struct gapi_domainParticipantListener));
}

gapi_conditionSeq *
gapi_conditionSeq__alloc (
    void
    )
{
    return (gapi_conditionSeq *)gapi_sequence_malloc ();
}

gapi_condition *
gapi_conditionSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_condition *)gapi_sequence_allocbuf (NULL, sizeof (gapi_condition), len);
}

gapi_sampleStateSeq *
gapi_sampleStateSeq__alloc (
    void
    )
{
    return (gapi_sampleStateSeq *)gapi_sequence_malloc ();
}

gapi_sampleStateKind *
gapi_sampleStateSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_sampleStateKind *)gapi_sequence_allocbuf (NULL, sizeof (gapi_sampleStateKind), len);
}

gapi_viewStateSeq *
gapi_viewStateSeq__alloc (
    void
    )
{
    return (gapi_viewStateSeq *)gapi_sequence_malloc ();
}

gapi_viewStateKind *
gapi_viewStateSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_viewStateKind *)gapi_sequence_allocbuf (NULL, sizeof (gapi_viewStateKind), len);
}

gapi_instanceStateSeq *
gapi_instanceStateSeq__alloc (
    void
    )
{
    return (gapi_instanceStateSeq *)gapi_sequence_malloc ();
}

gapi_instanceStateKind *
gapi_instanceStateSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_instanceStateKind *)gapi_sequence_allocbuf (NULL, sizeof (gapi_instanceStateKind), len);
}

gapi_octetSeq *
gapi_octetSeq__alloc (
    void
    )
{
    return (gapi_octetSeq *)gapi_sequence_malloc ();
}

gapi_octet *
gapi_octetSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_octet *)gapi_sequence_allocbuf (NULL, sizeof (gapi_octet), len);
}

void
gapi_domainParticipantFactoryQos_free (
    void *object
    )
{
}

gapi_domainParticipantFactoryQos *
gapi_domainParticipantFactoryQos__alloc (
    void
    )
{
    return (gapi_domainParticipantFactoryQos *)gapi__malloc (gapi_domainParticipantFactoryQos_free, 0, sizeof (gapi_domainParticipantFactoryQos));
}

void
gapi_domainParticipantQos_free (
    void *object
    )
{
    gapi_domainParticipantQos *o = (gapi_domainParticipantQos *)object;

    gapi_free (o->user_data.value._buffer);
}

gapi_domainParticipantQos *
gapi_domainParticipantQos__alloc (
    void
    )
{
    return (gapi_domainParticipantQos *)gapi__malloc (gapi_domainParticipantQos_free, 0, sizeof (gapi_domainParticipantQos));
}

void
gapi_topicQos_free (
    void *object
    )
{
    gapi_topicQos *o = (gapi_topicQos *)object;

    gapi_free (o->topic_data.value._buffer);
}

gapi_topicQos *
gapi_topicQos__alloc (
    void
    )
{
    gapi_topicQos *_this = NULL;

    _this = gapi__malloc (gapi_topicQos_free, 0, sizeof (gapi_topicQos));

    assert(gapi_sequence_is_valid(&_this->topic_data));
    return _this;
}

void
gapi_dataWriterQos_free (
    void *object
    )
{
    gapi_dataWriterQos *o = (gapi_dataWriterQos *)object;

    gapi_free (o->user_data.value._buffer);
}

gapi_dataWriterQos *
gapi_dataWriterQos__alloc (
    void
    )
{
    return (gapi_dataWriterQos *)gapi__malloc (gapi_dataWriterQos_free, 0, sizeof (gapi_dataWriterQos));
}

void
gapi_publisherQos_free (
    void *object
    )
{
    gapi_publisherQos *o = (gapi_publisherQos *)object;

    gapi_free (o->partition.name._buffer);
    gapi_free (o->group_data.value._buffer);
}

gapi_publisherQos *
gapi_publisherQos__alloc (
    void
    )
{
    return (gapi_publisherQos *)gapi__malloc (gapi_publisherQos_free, 0, sizeof (gapi_publisherQos));
}

void
gapi_dataReaderQos_free (
    void *object
    )
{
    gapi_dataReaderQos *o = (gapi_dataReaderQos *)object;

    gapi_free (o->user_data.value._buffer);
    gapi_free (o->subscription_keys.key_list._buffer);
    gapi_free (o->share.name);
}

gapi_dataReaderQos *
gapi_dataReaderQos__alloc (
    void
    )
{
    return (gapi_dataReaderQos *)gapi__malloc (gapi_dataReaderQos_free, 0, sizeof (gapi_dataReaderQos));
}

void
gapi_dataReaderViewQos_free (
    void *object
    )
{
    gapi_dataReaderViewQos *o = (gapi_dataReaderViewQos *)object;

    gapi_free (o->view_keys.key_list._buffer);
}

gapi_dataReaderViewQos *
gapi_dataReaderViewQos__alloc (
    void
    )
{
    return (gapi_dataReaderViewQos *)gapi__malloc (gapi_dataReaderViewQos_free, 0, sizeof (gapi_dataReaderViewQos));
}

void
gapi_subscriberQos_free (
    void *object
    )
{
    gapi_subscriberQos *o = (gapi_subscriberQos *)object;

    gapi_free (o->partition.name._buffer);
    gapi_free (o->group_data.value._buffer);
    gapi_free (o->share.name);
}

gapi_subscriberQos *
gapi_subscriberQos__alloc (
    void
    )
{
    return (gapi_subscriberQos *)gapi__malloc (gapi_subscriberQos_free, 0, sizeof (gapi_subscriberQos));
}

void
gapi_participantBuiltinTopicData_free (
    void *object
    )
{
    gapi_participantBuiltinTopicData *o = (gapi_participantBuiltinTopicData *)object;

    gapi_free (o->user_data.value._buffer);
}

gapi_participantBuiltinTopicData *
gapi_participantBuiltinTopicData__alloc (
    void
    )
{
    return (gapi_participantBuiltinTopicData *)gapi__malloc (gapi_participantBuiltinTopicData_free, 0, sizeof (gapi_participantBuiltinTopicData));
}

gapi_participantBuiltinTopicDataSeq *
gapi_participantBuiltinTopicDataSeq__alloc (
    void
    )
{
    return (gapi_participantBuiltinTopicDataSeq *)gapi_sequence_malloc ();
}

void
gapi_participantBuiltinTopicDataSeq_freebuf (
    void *buffer
    )
{
    gapi_participantBuiltinTopicData *b = (gapi_participantBuiltinTopicData *)buffer;
    gapi_unsigned_long *count = (gapi_unsigned_long *)gapi__header (buffer);
    gapi_unsigned_long i;

    for (i = 0; i < *count; i++) {
        gapi_participantBuiltinTopicData_free (&b[i]);
    }
}

gapi_participantBuiltinTopicData *
gapi_participantBuiltinTopicDataSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_participantBuiltinTopicData *)gapi_sequence_allocbuf (gapi_participantBuiltinTopicDataSeq_freebuf, sizeof (gapi_participantBuiltinTopicData), len);
}

void
gapi_topicBuiltinTopicData_free (
    void *object
    )
{
    gapi_topicBuiltinTopicData *o = (gapi_topicBuiltinTopicData *)object;

    gapi_free (o->name);
    gapi_free (o->type_name);
    gapi_free (o->topic_data.value._buffer);
}

gapi_topicBuiltinTopicData *
gapi_topicBuiltinTopicData__alloc (
    void
    )
{
    return (gapi_topicBuiltinTopicData *)gapi__malloc (gapi_topicBuiltinTopicData_free, 0, sizeof (gapi_topicBuiltinTopicData));
}

gapi_topicBuiltinTopicDataSeq *
gapi_topicBuiltinTopicDataSeq__alloc (
    void
    )
{
    return (gapi_topicBuiltinTopicDataSeq *)gapi_sequence_malloc ();
}

void
gapi_topicBuiltinTopicDataSeq_freebuf (
    void *buffer
    )
{
    gapi_topicBuiltinTopicData *b = (gapi_topicBuiltinTopicData *)buffer;
    gapi_unsigned_long *count = (gapi_unsigned_long *)gapi__header (buffer);
    gapi_unsigned_long i;

    for (i = 0; i < *count; i++) {
        gapi_topicBuiltinTopicData_free (&b[i]);
    }
}

gapi_topicBuiltinTopicData *
gapi_topicBuiltinTopicDataSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_topicBuiltinTopicData *)gapi_sequence_allocbuf (gapi_topicBuiltinTopicDataSeq_freebuf, sizeof (gapi_topicBuiltinTopicData), len);
}

void
gapi_publicationBuiltinTopicData_free (
    void *object
    )
{
    gapi_publicationBuiltinTopicData *o = (gapi_publicationBuiltinTopicData *)object;

    gapi_free (o->topic_name);
    gapi_free (o->type_name);
    gapi_free (o->partition.name._buffer);
    gapi_free (o->user_data.value._buffer);
    gapi_free (o->topic_data.value._buffer);
    gapi_free (o->group_data.value._buffer);
}

gapi_publicationBuiltinTopicData *
gapi_publicationBuiltinTopicData__alloc (
    void
    )
{
    return (gapi_publicationBuiltinTopicData *)gapi__malloc (gapi_publicationBuiltinTopicData_free, 0, sizeof (gapi_publicationBuiltinTopicData));
}

gapi_publicationBuiltinTopicDataSeq *
gapi_publicationBuiltinTopicDataSeq__alloc (
    void
    )
{
    return (gapi_publicationBuiltinTopicDataSeq *)gapi_sequence_malloc ();
}

void
gapi_publicationBuiltinTopicDataSeq_freebuf (
    void *buffer
    )
{
    gapi_publicationBuiltinTopicData *b = (gapi_publicationBuiltinTopicData *)buffer;
    gapi_unsigned_long *count = (gapi_unsigned_long *)gapi__header (buffer);
    gapi_unsigned_long i;

    for (i = 0; i < *count; i++) {
        gapi_publicationBuiltinTopicData_free (&b[i]);
    }
}

gapi_publicationBuiltinTopicData *
gapi_publicationBuiltinTopicDataSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_publicationBuiltinTopicData *)gapi_sequence_allocbuf (gapi_publicationBuiltinTopicDataSeq_freebuf, sizeof (gapi_publicationBuiltinTopicData), len);
}

void
gapi_subscriptionBuiltinTopicData_free (
    void *object
    )
{
    gapi_subscriptionBuiltinTopicData *o = (gapi_subscriptionBuiltinTopicData *)object;

    gapi_free (o->topic_name);
    gapi_free (o->type_name);
    gapi_free (o->partition.name._buffer);
    gapi_free (o->user_data.value._buffer);
    gapi_free (o->topic_data.value._buffer);
    gapi_free (o->group_data.value._buffer);
}

gapi_subscriptionBuiltinTopicData *
gapi_subscriptionBuiltinTopicData__alloc (
    void
    )
{
    return (gapi_subscriptionBuiltinTopicData *)gapi__malloc (gapi_subscriptionBuiltinTopicData_free, 0, sizeof (gapi_subscriptionBuiltinTopicData));
}

gapi_subscriptionBuiltinTopicDataSeq *
gapi_subscriptionBuiltinTopicDataSeq__alloc (
    void
    )
{
    return (gapi_subscriptionBuiltinTopicDataSeq *)gapi_sequence_malloc ();
}

void
gapi_subscriptionBuiltinTopicDataSeq_freebuf (
    void *buffer
    )
{
    gapi_subscriptionBuiltinTopicData *b = (gapi_subscriptionBuiltinTopicData *)buffer;
    gapi_unsigned_long *count = (gapi_unsigned_long *)gapi__header (buffer);
    gapi_unsigned_long i;

    for (i = 0; i < *count; i++) {
        gapi_subscriptionBuiltinTopicData_free (&b[i]);
    }
}

gapi_subscriptionBuiltinTopicData *
gapi_subscriptionBuiltinTopicDataSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_subscriptionBuiltinTopicData *)gapi_sequence_allocbuf (gapi_subscriptionBuiltinTopicDataSeq_freebuf, sizeof (gapi_subscriptionBuiltinTopicData), len);
}

gapi_sampleInfoSeq *
gapi_sampleInfoSeq__alloc (
    void
    )
{
    return (gapi_sampleInfoSeq *)gapi_sequence_malloc ();
}

gapi_sampleInfo *
gapi_sampleInfoSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_sampleInfo *)gapi_sequence_allocbuf (NULL, sizeof (gapi_sampleInfo), len);
}


gapi_dataSampleSeq *
gapi_dataSampleSeq__alloc (
    void
    )
{
    return (gapi_dataSampleSeq *)gapi_sequence_malloc ();
}

gapi_dataSample *
gapi_dataSampleSeq_allocbuf (
    gapi_unsigned_long len
    )
{
    return (gapi_dataSample *)gapi_sequence_allocbuf(NULL, sizeof (gapi_dataSample), len);
}
