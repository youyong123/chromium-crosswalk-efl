// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/system/channel_endpoint.h"

#include "base/logging.h"
#include "mojo/system/channel.h"
#include "mojo/system/message_pipe.h"

namespace mojo {
namespace system {

ChannelEndpoint::ChannelEndpoint(MessagePipe* message_pipe, unsigned port)
    : state_(STATE_NORMAL),
      message_pipe_(message_pipe),
      port_(port),
      channel_(),
      local_id_(MessageInTransit::kInvalidEndpointId),
      remote_id_(MessageInTransit::kInvalidEndpointId) {
  DCHECK(message_pipe_.get());
  DCHECK(port_ == 0 || port_ == 1);
}

void ChannelEndpoint::AttachToChannel(Channel* channel,
                                      MessageInTransit::EndpointId local_id) {
  DCHECK(channel);
  DCHECK_NE(local_id, MessageInTransit::kInvalidEndpointId);

  base::AutoLock locker(lock_);
  DCHECK(!channel_);
  DCHECK_EQ(local_id_, MessageInTransit::kInvalidEndpointId);
  channel_ = channel;
  local_id_ = local_id;
}

void ChannelEndpoint::Run(MessageInTransit::EndpointId remote_id) {
  DCHECK_NE(remote_id, MessageInTransit::kInvalidEndpointId);

  base::AutoLock locker(lock_);
  DCHECK(channel_);
  DCHECK_EQ(remote_id_, MessageInTransit::kInvalidEndpointId);
  remote_id_ = remote_id;
}

void ChannelEndpoint::DetachFromChannel() {
  base::AutoLock locker(lock_);
  DCHECK(channel_);
  DCHECK_NE(local_id_, MessageInTransit::kInvalidEndpointId);
  // TODO(vtl): Once we combine "run" into "attach", |remote_id_| should valid
  // here as well.
  channel_ = NULL;
  local_id_ = MessageInTransit::kInvalidEndpointId;
  remote_id_ = MessageInTransit::kInvalidEndpointId;
}

ChannelEndpoint::~ChannelEndpoint() {
  DCHECK(!channel_);
  DCHECK_EQ(local_id_, MessageInTransit::kInvalidEndpointId);
  DCHECK_EQ(remote_id_, MessageInTransit::kInvalidEndpointId);
}

}  // namespace system
}  // namespace mojo
