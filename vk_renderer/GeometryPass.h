#pragma once

#include "Pass.h"
#include "GBuffer.h"


class GeometryPass : public Pass {
	using Pass::Pass;

public:
	virtual void updateBufferData() override;

	virtual VkCommandBuffer getCurrentCommandBuffer() const override { return commandBuffer; }
	virtual GBuffer* getGBuffer() override { return &gBuffer; }

private:
	GBuffer gBuffer;
	VkCommandBuffer commandBuffer;

	virtual void initAttachments() override;
	virtual void initCommandBuffers() override;
	virtual void initDescriptorSet() override;
	virtual void initDescriptorSetLayout() override;
	virtual void initGraphicsPipeline() override;
	virtual void initUniformBuffer() override;
};