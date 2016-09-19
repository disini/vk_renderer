#pragma once

#include "Pass.h"


class GeomPass : public Pass {
	using Pass::Pass;

public:
	virtual void updateData() override;

private:
	virtual void initAttachments() override;
	virtual void initCommandBuffers() override;
	virtual void initFramebuffers() override;
	virtual void initDescriptorSet() override;
};