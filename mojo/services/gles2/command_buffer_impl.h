// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_GLES2_COMMAND_BUFFER_IMPL_H_
#define MOJO_SERVICES_GLES2_COMMAND_BUFFER_IMPL_H_

#include "base/memory/scoped_ptr.h"
#include "base/timer/timer.h"
#include "mojo/public/cpp/system/core.h"
#include "mojo/services/gles2/command_buffer.mojom.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"

namespace gpu {
class CommandBufferService;
class GpuScheduler;
class GpuControlService;
namespace gles2 {
class GLES2Decoder;
class MailboxManager;
}
}

namespace gfx {
class GLContext;
class GLShareGroup;
class GLSurface;
}

namespace mojo {

class CommandBufferImpl : public InterfaceImpl<CommandBuffer> {
 public:
  // Offscreen.
  CommandBufferImpl(gfx::GLShareGroup* share_group,
                    gpu::gles2::MailboxManager* mailbox_manager);
  // Onscreen.
  CommandBufferImpl(gfx::AcceleratedWidget widget,
                    const gfx::Size& size,
                    gfx::GLShareGroup* share_group,
                    gpu::gles2::MailboxManager* mailbox_manager);
  virtual ~CommandBufferImpl();

  virtual void Initialize(CommandBufferSyncClientPtr sync_client,
                          mojo::ScopedSharedBufferHandle shared_state) OVERRIDE;
  virtual void SetGetBuffer(int32_t buffer) OVERRIDE;
  virtual void Flush(int32_t put_offset) OVERRIDE;
  virtual void MakeProgress(int32_t last_get_offset) OVERRIDE;
  virtual void RegisterTransferBuffer(
      int32_t id,
      mojo::ScopedSharedBufferHandle transfer_buffer,
      uint32_t size) OVERRIDE;
  virtual void DestroyTransferBuffer(int32_t id) OVERRIDE;
  virtual void Echo(const Callback<void()>& callback) OVERRIDE;

 private:
  bool DoInitialize(mojo::ScopedSharedBufferHandle shared_state);

  void OnResize(gfx::Size size, float scale_factor);

  void OnParseError();

  CommandBufferSyncClientPtr sync_client_;

  gfx::AcceleratedWidget widget_;
  gfx::Size size_;
  scoped_ptr<gpu::CommandBufferService> command_buffer_;
  scoped_ptr<gpu::gles2::GLES2Decoder> decoder_;
  scoped_ptr<gpu::GpuScheduler> scheduler_;
  scoped_refptr<gfx::GLContext> context_;
  scoped_refptr<gfx::GLSurface> surface_;
  scoped_refptr<gfx::GLShareGroup> share_group_;
  scoped_refptr<gpu::gles2::MailboxManager> mailbox_manager_;

  DISALLOW_COPY_AND_ASSIGN(CommandBufferImpl);
};

}  // namespace mojo

#endif  // MOJO_SERVICES_GLES2_COMMAND_BUFFER_IMPL_H_
