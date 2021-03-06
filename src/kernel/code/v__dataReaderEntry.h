/*
 *                         OpenSplice DDS
 *
 *   This software and documentation are Copyright 2006 to TO_YEAR PrismTech
 *   Limited, its affiliated companies and licensors. All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#ifndef V__DATAREADERENTRY_H
#define V__DATAREADERENTRY_H

#include "v_dataReaderEntry.h"

/**
 * \brief Set the specified flags in the instanceState of all DataReader
 * instances associated with the specified DataReaderEntry.
 *
 * \param _this The DataReaderEntry for which all instanceStates must be updated.
 * \param flags The flags that must be set for all instanceStates associated with
 *              _this.
 */
void
v_dataReaderEntryMarkInstanceStates (
    v_dataReaderEntry _this,
    c_ulong flags);

/**
 * \brief Reset the specified flags in the instanceState of all DataReader
 *  instances associated with the specified DataReaderEntry.
 *
 * \param _this The DataReaderEntry for which all instanceStates must be updated.
 * \param flags The flags that must be reset for all instanceStates associated with
 *              _this.
 */
void
v_dataReaderEntryUnmarkInstanceStates (
    v_dataReaderEntry _this,
    c_ulong flags);

v_writeResult
v_dataReaderEntryWriteHistoricalTransaction(
    v_dataReaderEntry _this,
    v_transaction *transaction,
    v_message message,
    v_dataReaderInstance instance);

void
v_dataReaderEntryFlushTransactionNoLock(
    v_instance instance,
    v_message message,
    c_voidp arg);

v_typeRepresentation
v__dataReaderEntryGetTypeRepresentation (
    v_dataReaderEntry _this);

v_writeResult
v_dataReaderEntryDisposeAll (
    v_dataReaderEntry _this,
    v_message disposeMsg,
    c_bool (*condition)(c_object instance, c_voidp condition_arg),
    c_voidp condition_arg);

#endif /* V__DATAREADERENTRY_H */
