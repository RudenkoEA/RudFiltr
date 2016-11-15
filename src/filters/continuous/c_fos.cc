#include "c_fos.h"


namespace Filters
{

namespace Continuous
{


using Math::Rand::gaussianVector;


FOS::FOS(Core::PtrFilterParameters params, Core::PtrTask task)
    : ContinuousFilter(params, task)
{
    m_info->setName(m_task->info()->type() + "ФОСн (" + std::to_string(task->dimX()) + ")");
}

void FOS::algorithm()
{
    double sqrtdt = std::sqrt(m_params->integrationStep());
    Vector dy;
    Matrix Gamma;

    for (size_t n = 1; n < m_result.size(); ++n) { // tn = t0 + n * dt
        m_task->setTime(m_result[n - 1].time);
        Gamma = m_result[n - 1].varX - m_result[n - 1].varZ;

        for (size_t s = 0; s < m_params->sampleSize(); ++s) {
            m_sampleX[s] = m_sampleX[s] + m_task->a(m_sampleX[s]) * m_params->integrationStep() +
                           m_task->B(m_sampleX[s]) * gaussianVector(m_task->dimV(), 0.0, sqrtdt);

            dy = m_task->c(m_sampleX[s]) * m_params->integrationStep() +
                 m_task->D(m_sampleX[s]) * gaussianVector(m_task->dimW(), 0.0, sqrtdt);
            m_sampleY[s] = m_sampleY[s] + dy;

            m_sampleZ[s] =
                m_sampleZ[s] + m_task->a(m_sampleZ[s]) * m_params->integrationStep() +
                m_task->K(m_sampleZ[s], Gamma) * (dy - m_task->c(m_sampleZ[s]) * m_params->integrationStep());
        }
        writeResult(n);
    }
}


} // end Filters::Continuous

} // end Filters
