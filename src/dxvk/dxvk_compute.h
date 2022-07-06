#pragma once

#include <vector>

#include "../util/sync/sync_list.h"

#include "dxvk_bind_mask.h"
#include "dxvk_graphics_state.h"
#include "dxvk_pipecache.h"
#include "dxvk_pipelayout.h"
#include "dxvk_resource.h"
#include "dxvk_shader.h"
#include "dxvk_stats.h"

namespace dxvk {
  
  class DxvkDevice;
  class DxvkStateCache;
  class DxvkPipelineManager;
  struct DxvkPipelineStats;


  /**
   * \brief Shaders used in compute pipelines
   */
  struct DxvkComputePipelineShaders {
    Rc<DxvkShader> cs;

    bool eq(const DxvkComputePipelineShaders& other) const {
      return cs == other.cs;
    }

    size_t hash() const {
      return DxvkShader::getHash(cs);
    }
  };


  /**
   * \brief Compute pipeline instance
   */
  struct DxvkComputePipelineInstance {
    DxvkComputePipelineInstance() { }
    DxvkComputePipelineInstance(
      const DxvkComputePipelineStateInfo& state_,
            VkPipeline                    handle_)
    : state(state_), handle(handle_) { }

    DxvkComputePipelineStateInfo state;
    VkPipeline                   handle = VK_NULL_HANDLE;
  };
  
  
  /**
   * \brief Compute pipeline
   * 
   * Stores a compute pipeline object and the corresponding
   * pipeline layout. Unlike graphics pipelines, compute
   * pipelines do not need to be recompiled against any sort
   * of pipeline state.
   */
  class DxvkComputePipeline {
    
  public:
    
    DxvkComputePipeline(
            DxvkDevice*                 device,
            DxvkPipelineManager*        pipeMgr,
            DxvkComputePipelineShaders  shaders,
            DxvkBindingLayoutObjects*   layout,
            DxvkShaderPipelineLibrary*  library);

    ~DxvkComputePipeline();
    
    /**
     * \brief Shaders used by the pipeline
     * \returns Shaders used by the pipeline
     */
    const DxvkComputePipelineShaders& shaders() const {
      return m_shaders;
    }
    
    /**
     * \brief Pipeline layout
     * 
     * Stores the pipeline layout and the descriptor set
     * layouts, as well as information on the resource
     * slots used by the pipeline.
     * \returns Pipeline layout
     */
    DxvkBindingLayoutObjects* getBindings() const {
      return m_bindings;
    }
    
    /**
     * \brief Retrieves pipeline handle
     * 
     * \param [in] state Pipeline state
     * \returns Pipeline handle
     */
    VkPipeline getPipelineHandle(
      const DxvkComputePipelineStateInfo& state);
    
    /**
     * \brief Compiles a pipeline
     * 
     * Asynchronously compiles the given pipeline
     * and stores the result for future use.
     * \param [in] state Pipeline state
     */
    void compilePipeline(
      const DxvkComputePipelineStateInfo& state);
    
  private:
    
    DxvkDevice*                 m_device;    
    DxvkPipelineCache*          m_cache;
    DxvkStateCache*             m_stateCache;
    DxvkPipelineStats*          m_stats;

    DxvkShaderPipelineLibrary*  m_library;
    VkPipeline                  m_libraryHandle;

    DxvkComputePipelineShaders  m_shaders;
    DxvkBindingLayoutObjects*   m_bindings;
    
    alignas(CACHE_LINE_SIZE)
    dxvk::mutex                             m_mutex;
    sync::List<DxvkComputePipelineInstance> m_pipelines;
    
    DxvkComputePipelineInstance* createInstance(
      const DxvkComputePipelineStateInfo& state);
    
    DxvkComputePipelineInstance* findInstance(
      const DxvkComputePipelineStateInfo& state);
    
    VkPipeline createPipeline(
      const DxvkComputePipelineStateInfo& state) const;
    
    void destroyPipeline(
            VkPipeline                    pipeline);

    void writePipelineStateToCache(
      const DxvkComputePipelineStateInfo& state) const;
    
  };
  
}