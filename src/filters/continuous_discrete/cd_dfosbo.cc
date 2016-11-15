#include "cd_dfosbo.h"


namespace Filters
{

namespace ContinuousDiscrete
{


using Math::Rand::gaussianVector;
using Math::LinAlg::PinvSVD;
using Math::Statistic::Cov;
using Math::Statistic::Var;
using Math::Statistic::Mean;


DFOSBO::DFOSBO(Core::PtrFilterParameters params, Core::PtrTask task)
    : ContinuousDiscreteFilter(params, task)
{
    long p = task->dimY() * long(params->orderMult());
    m_info->setName(m_task->info()->type() + "ФКПнд-дп (" + std::to_string(p) + ")");
}

void DFOSBO::init()
{
    Filter::init();
    Zb.resize(m_params->sampleSize());
}

void DFOSBO::zeroIteration()
{
    ContinuousDiscreteFilter::zeroIteration(); // тут вычисляются общие для всех фильтров H0, e0 и
                                               // т.д. и заполняется результат 0-го шага

    Zb.resize(m_params->sampleSize());

    ny = uint(m_task->dimY());
    p  = ny * m_params->orderMult();

    for (size_t s = 0; s < m_params->sampleSize(); ++s) {
        Zb[s] = Vector::Zero(int(p));
        for (long i = 0; i < long(ny); ++i) {
            Zb[s][i] = m_sampleY[s][i];
        }
    }
}

void DFOSBO::algorithm()
{
    double sqrtdt = std::sqrt(m_params->integrationStep());
    Vector h, kappa, mZb;
    Matrix G, F, T, Gamma, DZb, DxZb;

    // n = 1 .. m_params->measurementCount() * m_params->predictionCount() * m_params->integrationCount()
    for (size_t n = 1; n < m_result.size(); ++n) {
        m_task->setTime(m_result[n - 1].time);

        for (size_t s = 0; s < m_params->sampleSize(); ++s) {
            m_sampleX[s] = m_sampleX[s] + m_task->a(m_sampleX[s]) * m_params->integrationStep() +
                           m_task->B(m_sampleX[s]) * gaussianVector(m_task->dimV(), 0.0, sqrtdt);
        }
        writeResult(n, true); //  mX, DX вычисляются, а mZ, DZ, mE, DE копируются из предыдущего

        if (n % m_params->integrationCount() == 0) {
            DxZb  = Cov(m_sampleX, Zb);
            mZb   = Mean(Zb);
            DZb   = Var(Zb, mZb);
            Gamma = DxZb * PinvSVD(DZb);
            kappa = m_result[n].meanX - Gamma * mZb;
            if (n % (m_params->predictionCount() * m_params->integrationCount()) == 0) {
                T = m_result[n].varX - Gamma * DxZb.transpose();
            }
            for (size_t s = 0; s < m_params->sampleSize(); ++s) {
                m_sampleZ[s] = Gamma * Zb[s] + kappa;
            }
            writeResult(n);
        }

        if (n % (m_params->predictionCount() * m_params->integrationCount()) == 0) {
            for (size_t s = 0; s < m_params->sampleSize(); ++s) {
                m_sampleY[s] = m_task->c(m_sampleX[s]); // = Yk (Y в момент t = tk)

                h = m_task->h(m_sampleZ[s], T);
                G = m_task->G(m_sampleZ[s], T);
                F = m_task->F(m_sampleZ[s], T);

                m_sampleZ[s] = m_sampleZ[s] + T * G.transpose() * PinvSVD(F) * (m_sampleY[s] - h);

                for (long i = long(p - 1); i >= long(ny); --i) {
                    Zb[s][i] = Zb[s][i - long(ny)];
                }
                for (long i = 0; i < long(ny); ++i) {
                    Zb[s][i] = m_sampleY[s][i];
                }
            }
            writeResult(n);
        }
    }
}


} // end ContinuousDiscrete

} // end Filters
