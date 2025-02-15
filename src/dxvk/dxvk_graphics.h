#pragma once

#include <mutex>

#include "../util/sync/sync_list.h"

#include "dxvk_bind_mask.h"
#include "dxvk_constant_state.h"
#include "dxvk_graphics_state.h"
#include "dxvk_pipecache.h"
#include "dxvk_pipelayout.h"
#include "dxvk_renderpass.h"
#include "dxvk_resource.h"
#include "dxvk_shader.h"
#include "dxvk_stats.h"

namespace dxvk {
  
  class DxvkDevice;
  class DxvkPipelineManager;

  /**
   * \brief Flags that describe pipeline properties
   */
  enum class DxvkGraphicsPipelineFlag {
    HasTransformFeedback,
    HasStorageDescriptors,
  };

  using DxvkGraphicsPipelineFlags = Flags<DxvkGraphicsPipelineFlag>;


  /**
   * \brief Shaders used in graphics pipelines
   */
  struct DxvkGraphicsPipelineShaders {
    Rc<DxvkShader> vs;
    Rc<DxvkShader> tcs;
    Rc<DxvkShader> tes;
    Rc<DxvkShader> gs;
    Rc<DxvkShader> fs;

    bool eq(const DxvkGraphicsPipelineShaders& other) const {
      return vs == other.vs && tcs == other.tcs
          && tes == other.tes && gs == other.gs
          && fs == other.fs;
    }

    size_t hash() const {
      DxvkHashState state;
      state.add(DxvkShader::getHash(vs));
      state.add(DxvkShader::getHash(tcs));
      state.add(DxvkShader::getHash(tes));
      state.add(DxvkShader::getHash(gs));
      state.add(DxvkShader::getHash(fs));
      return state;
    }

    bool validate() const {
      return validateShaderType(vs, VK_SHADER_STAGE_VERTEX_BIT)
          && validateShaderType(tcs, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
          && validateShaderType(tes, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
          && validateShaderType(gs, VK_SHADER_STAGE_GEOMETRY_BIT)
          && validateShaderType(fs, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    static bool validateShaderType(const Rc<DxvkShader>& shader, VkShaderStageFlagBits stage) {
      return shader == nullptr || shader->info().stage == stage;
    }
  };
  
  
  /**
   * \brief Common graphics pipeline state
   * 
   * Non-dynamic pipeline state that cannot
   * be changed dynamically.
   */
  struct DxvkGraphicsCommonPipelineStateInfo {
    bool                                msSampleShadingEnable;
    float                               msSampleShadingFactor;
  };
  
  
  /**
   * \brief Graphics pipeline instance
   * 
   * Stores a state vector and the
   * corresponding pipeline handle.
   */
  class DxvkGraphicsPipelineInstance {

  public:

    DxvkGraphicsPipelineInstance()
    : m_stateVector (),
      m_pipeline    (VK_NULL_HANDLE) { }

    DxvkGraphicsPipelineInstance(
      const DxvkGraphicsPipelineStateInfo&  state,
            VkPipeline                      pipe)
    : m_stateVector (state),
      m_pipeline    (pipe) { }

    /**
     * \brief Checks for matching pipeline state
     * 
     * \param [in] stateVector Graphics pipeline state
     * \returns \c true if the specialization is compatible
     */
    bool isCompatible(
      const DxvkGraphicsPipelineStateInfo&  state) {
      return m_stateVector == state;
    }

    /**
     * \brief Retrieves pipeline
     * \returns The pipeline handle
     */
    VkPipeline pipeline() const {
      return m_pipeline;
    }

  private:

    DxvkGraphicsPipelineStateInfo m_stateVector;
    VkPipeline                    m_pipeline;

  };

  
  /**
   * \brief Graphics pipeline
   * 
   * Stores the pipeline layout as well as methods to
   * recompile the graphics pipeline against a given
   * pipeline state vector.
   */
  class DxvkGraphicsPipeline {
    
  public:
    
    DxvkGraphicsPipeline(
            DxvkPipelineManager*        pipeMgr,
            DxvkGraphicsPipelineShaders shaders,
            DxvkBindingLayoutObjects*   layout);

    ~DxvkGraphicsPipeline();

    /**
     * \brief Shaders used by the pipeline
     * \returns Shaders used by the pipeline
     */
    const DxvkGraphicsPipelineShaders& shaders() const {
      return m_shaders;
    }
    
    /**
     * \brief Returns graphics pipeline flags
     * \returns Graphics pipeline property flags
     */
    DxvkGraphicsPipelineFlags flags() const {
      return m_flags;
    }
    
    /**
     * \brief Pipeline layout
     * 
     * Stores the pipeline layout and the descriptor set
     * layout, as well as information on the resource
     * slots used by the pipeline.
     * \returns Pipeline layout
     */
    DxvkBindingLayoutObjects* getBindings() const {
      return m_bindings;
    }

    /**
     * \brief Queries shader for a given stage
     * 
     * In case no shader is specified for the
     * given stage, \c nullptr will be returned.
     * \param [in] stage The shader stage
     * \returns Shader of the given stage
     */
    Rc<DxvkShader> getShader(
            VkShaderStageFlagBits             stage) const;

    /**
     * \brief Queries global resource barrier
     *
     * Returns the stages that can access resources in this
     * pipeline with the given pipeline state, as well as
     * the ways in which resources are accessed. This does
     * not include render targets. The barrier is meant to
     * be executed after the render pass.
     * \returns Global barrier
     */
    DxvkGlobalPipelineBarrier getGlobalBarrier(
      const DxvkGraphicsPipelineStateInfo&    state) const;

    /**
     * \brief Pipeline handle
     * 
     * Retrieves a pipeline handle for the given pipeline
     * state. If necessary, a new pipeline will be created.
     * \param [in] state Pipeline state vector
     * \returns Pipeline handle
     */
    VkPipeline getPipelineHandle(
      const DxvkGraphicsPipelineStateInfo&    state);
    
    /**
     * \brief Compiles a pipeline
     * 
     * Asynchronously compiles the given pipeline
     * and stores the result for future use.
     * \param [in] state Pipeline state vector
     */
    void compilePipeline(
      const DxvkGraphicsPipelineStateInfo&    state);
    
  private:
    
    Rc<vk::DeviceFn>            m_vkd;
    DxvkPipelineManager*        m_pipeMgr;

    DxvkGraphicsPipelineShaders m_shaders;
    DxvkBindingLayoutObjects*   m_bindings;
    
    uint32_t m_vsIn  = 0;
    uint32_t m_fsOut = 0;
    
    DxvkGlobalPipelineBarrier           m_barrier;
    DxvkGraphicsPipelineFlags           m_flags;
    DxvkGraphicsCommonPipelineStateInfo m_common;
    
    // List of pipeline instances, shared between threads
    alignas(CACHE_LINE_SIZE)
    dxvk::mutex                               m_mutex;
    sync::List<DxvkGraphicsPipelineInstance>  m_pipelines;
    
    DxvkGraphicsPipelineInstance* createInstance(
      const DxvkGraphicsPipelineStateInfo& state);
    
    DxvkGraphicsPipelineInstance* findInstance(
      const DxvkGraphicsPipelineStateInfo& state);
    
    VkPipeline createPipeline(
      const DxvkGraphicsPipelineStateInfo& state) const;
    
    void destroyPipeline(
            VkPipeline                     pipeline) const;
    
    DxvkShaderModule createShaderModule(
      const Rc<DxvkShader>&                shader,
      const DxvkGraphicsPipelineStateInfo& state) const;
    
    Rc<DxvkShader> getPrevStageShader(
            VkShaderStageFlagBits          stage) const;

    bool validatePipelineState(
      const DxvkGraphicsPipelineStateInfo& state,
            bool                           trusted) const;
    
    void writePipelineStateToCache(
      const DxvkGraphicsPipelineStateInfo& state) const;
    
    void logPipelineState(
            LogLevel                       level,
      const DxvkGraphicsPipelineStateInfo& state) const;

  };
  
}