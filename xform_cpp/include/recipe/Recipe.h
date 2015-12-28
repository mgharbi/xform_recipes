/* -----------------------------------------------------------------
 * File:    Recipe.h
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-24
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/


#ifndef RECIPE_H_0ZV3UECD
#define RECIPE_H_0ZV3UECD

#include <vector>
#include <memory>

#include "utils/static_image.h"
#include "global_parameters.h"


namespace xform{

class Recipe
{
public:
    Recipe( Image<uint32_t>& unprocessed );

#ifndef __ANDROID__
    Recipe( Image<uint32_t>& unprocessed, Image<uint32_t>& processed );
    void fit( Image<uint32_t>& unprocessed, Image<uint32_t>& processed );
#endif

    void reconstruct_image( const Image<uint32_t>& unprocessed, Image<uint32_t> &output );

    void init();
    void quantize();

    void dequantize();
    void precompute_features( const Image<uint32_t>& unprocessed );
    void reconstruct_with_features(  Image<uint32_t> &output );

    std::shared_ptr<Image<uint32_t> > lowpass_residual() { return m_lp_residual; };
    std::shared_ptr<Image<float> > highpass_coefficients() { return m_hp_coefs; };
    std::vector<float> qtable() { return m_qtable; };

    int nLumaBands()  const { return XFORM_LUMA_BANDS; };
    int nPyrLevels()  const { return XFORM_PYR_LEVELS; };
    int nCoefMaps()   const { return nLumCoefs()+2*nChromCoefs(); }
    int nChromCoefs() const { return m_unprocessed_channels + 1; };
    int nLumCoefs()   const 
        { return (m_unprocessed_channels + 1) + (XFORM_LUMA_BANDS-1) + (XFORM_PYR_LEVELS - 1); };
    int nFeatureChannels()   const 
        { return (m_unprocessed_channels + 1) + (XFORM_PYR_LEVELS - 1); };

    void set_hp_coefs(Image<float> &hp_coefs) { m_hp_coefs = std::shared_ptr<Image<float> >(&hp_coefs); };
    void set_lp_residual(Image<uint32_t> &lp_residual) { m_lp_residual = std::shared_ptr<Image<uint32_t> >(&lp_residual); };
    void set_qtable(std::vector<float> &qtable) { m_qtable = qtable; };

private:
    
    #ifndef __ANDROID__
    void regression( Image<float>& features, Image<float>& target );
    #endif

    int m_unprocessed_channels,
        m_processed_channels;
    int m_model_width,
        m_model_height;
    int m_lp_width,
        m_lp_height;

    // Recipe parameters
    std::shared_ptr<Image<uint32_t> > m_lp_residual;
    std::shared_ptr<Image<float> >    m_hp_coefs;
    std::vector<float> m_qtable;

    // Features for the reconstruction
    Image<uint32_t> m_lp_unprocessed;
    Image<float> m_hp_unprocessed;
    Image<float> m_pyramid_unprocessed;

}; // class Recipe

} // namespace xform

#endif /* end of include guard: RECIPE_H_0ZV3UECD */
