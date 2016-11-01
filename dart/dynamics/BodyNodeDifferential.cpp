/*
 * Copyright (c) 2016, Graphics Lab, Georgia Tech Research Corporation
 * Copyright (c) 2016, Humanoid Lab, Georgia Tech Research Corporation
 * Copyright (c) 2016, Personal Robotics Lab, Carnegie Mellon University
 * All rights reserved.
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#include "dart/dynamics/BodyNodeDifferential.hpp"

#include "dart/dynamics/DegreeOfFreedom.hpp"
#include "dart/dynamics/BodyNode.hpp"

namespace dart {
namespace dynamics {

namespace detail {

//==============================================================================
BodyNodeDifferentialState::BodyNodeDifferentialState()
  : mV_q_q(std::vector<GradientMatrix>()),
    mV_q_dq(std::vector<GradientMatrix>()),
    mV_dq_dq(std::vector<GradientMatrix>())
{
  // Do nothing
}

} // namespace detail

//==============================================================================
//BodyNodeDifferential::BodyNodeDifferential(const StateData& state)
//{
//  mState = state;
//}

//==============================================================================
void BodyNodeDifferential::updateSpatialVelocityGradients()
{
  const auto* thisBodyNode = mComposite;
  const auto* thisJoint = thisBodyNode->getParentJoint();
  const auto* thisParentBodyNode = thisBodyNode->getParentBodyNode();

  if (thisParentBodyNode)
  {
    const auto* thisParentBodyNodeDifferential
        = thisParentBodyNode->get<BodyNodeDifferential>();

    mV_q
        = math::AdInvTJac(
          thisJoint->getRelativeTransform(),
          thisParentBodyNodeDifferential->mV_q);

    mV_dq
        = math::AdInvTJac(
          thisJoint->getRelativeTransform(),
          thisParentBodyNodeDifferential->mV_dq);
  }
  else
  {
    mV_q.setZero();
    mV_dq.setZero();
  }

  const auto numDofs = thisJoint->getNumDofs();

  if (numDofs == 0)
    return;

  const auto index = thisJoint->getDof(0)->getIndexInSkeleton();

  assert(thisJoint->getRelativeJacobian().rows() == 6);
  assert(thisJoint->getRelativeJacobian().cols() == numDofs);
  mV_q.block(0, index, 6, numDofs)
      += math::adJac(thisBodyNode->getSpatialVelocity(),
                     thisJoint->getRelativeJacobian());

  mV_dq.block(0, index, 6, numDofs) += thisJoint->getRelativeJacobian();
}

//==============================================================================
const BodyNodeDifferential::GradientMatrix&
BodyNodeDifferential::getSpatialVelocityDerivativeWrtPositions() const
{
  if (mNeedSpatialVelocityDerivativeWrtPositionsUpdate)
  {
    const auto* thisBodyNode = mComposite;
    const auto* thisJoint = thisBodyNode->getParentJoint();
    const auto* thisParentBodyNode = thisBodyNode->getParentBodyNode();

    if (thisParentBodyNode)
    {
      const auto* thisParentBodyNodeDifferential
          = thisParentBodyNode->get<BodyNodeDifferential>();

      mV_q = math::AdInvTJac(
            thisJoint->getRelativeTransform(),
            thisParentBodyNodeDifferential->mV_q);
    }
    else
    {
      mV_q.setZero();
    }

    const auto numDofs = thisJoint->getNumDofs();
    if (numDofs != 0)
    {
      const auto index = thisJoint->getDof(0)->getIndexInSkeleton();

      assert(thisJoint->getRelativeJacobian().rows() == 6);
      assert(thisJoint->getRelativeJacobian().cols() == numDofs);
      mV_q.block(0, index, 6, numDofs)
          += math::adJac(thisBodyNode->getSpatialVelocity(),
                         thisJoint->getRelativeJacobian());
    }

    mNeedSpatialVelocityDerivativeWrtPositionsUpdate = false;
  }

  return mV_q;
}

//==============================================================================
const BodyNodeDifferential::GradientMatrix&
BodyNodeDifferential::getSpatialVelocityDerivativeWrtVelocities() const
{
  if (mNeedSpatialVelocityDerivativeWrtVelocitiesUpdate)
  {
    const auto* thisBodyNode = mComposite;
    const auto* thisJoint = thisBodyNode->getParentJoint();
    const auto* thisParentBodyNode = thisBodyNode->getParentBodyNode();

    if (thisParentBodyNode)
    {
      const auto* thisParentBodyNodeDifferential
          = thisParentBodyNode->get<BodyNodeDifferential>();

      mV_dq
          = math::AdInvTJac(
            thisJoint->getRelativeTransform(),
            thisParentBodyNodeDifferential->mV_dq);
    }
    else
    {
      mV_dq.setZero();
    }

    const auto numDofs = thisJoint->getNumDofs();
    if (numDofs != 0)
    {
      const auto index = thisJoint->getDof(0)->getIndexInSkeleton();

      assert(thisJoint->getRelativeJacobian().rows() == 6);
      assert(thisJoint->getRelativeJacobian().cols() == numDofs);
      mV_dq.block(0, index, 6, numDofs) += thisJoint->getRelativeJacobian();
    }

    mNeedSpatialVelocityDerivativeWrtVelocitiesUpdate = false;
  }

  return mV_dq;
}

//==============================================================================
void BodyNodeDifferential::updateSpatialVelocityHessians()
{
  int rowIndex;
  int colIndex;

  const auto* thisBodyNode = mComposite;
  const auto* thisJoint = thisBodyNode->getParentJoint();
  const auto* thisParentBodyNode = thisBodyNode->getParentBodyNode();

  if (thisParentBodyNode)
  {
    const auto* thisParentBodyNodeDifferential
        = thisParentBodyNode->get<BodyNodeDifferential>();

    mV_q_q[rowIndex]
        = math::AdInvTJac(
          thisJoint->getRelativeTransform(),
          thisParentBodyNodeDifferential->mV_q_q[rowIndex]);

    mV_q_dq[rowIndex]
        = math::AdInvTJac(
          thisJoint->getRelativeTransform(),
          thisParentBodyNodeDifferential->mV_q_dq[rowIndex]);

    mV_dq_dq[rowIndex]
        = math::AdInvTJac(
          thisJoint->getRelativeTransform(),
          thisParentBodyNodeDifferential->mV_dq_dq[rowIndex]);
  }
  else
  {
    mV_q_q[rowIndex].setZero();
    mV_q_dq[rowIndex].setZero();
    mV_dq_dq[rowIndex].setZero();
  }

  const auto numDofs = thisJoint->getNumDofs();

  if (numDofs == 0)
    return;

  const auto index = thisJoint->getDof(0)->getIndexInSkeleton();

  mV_q.block(0, index, 6, numDofs)
      += math::adJac(thisBodyNode->getSpatialVelocity(),
                     thisJoint->getRelativeJacobian());

  mV_dq.block(0, index, 6, numDofs) += thisJoint->getRelativeJacobian();

  Eigen::Vector6d temp_q_q = Eigen::Vector6d::Zero();
  Eigen::Vector6d temp_q_dq = Eigen::Vector6d::Zero();

//  if (getIndexInSkeleton() == rowIndex)
//  {
//    temp_q_q += mV_q[colIndex];
//    temp_q_dq += mV_dq[colIndex];
//  }

//  if (getIndexInSkeleton() == colIndex)
//  {
//    temp_q_q += mV_q[rowIndex];
//  }

//  if (getIndexInSkeleton() == rowIndex && getIndexInSkeleton() == colIndex)
//  {
//    temp_q_q
//        += math::ad(thisJoint->getLocalJacobian(), getSpatialVelocity());
//  }

//  mV_q_q[rowIndex][colIndex]
//      -= math::ad(thisJoint->getLocalJacobian(), temp_q_q);
//  mV_q_dq[rowIndex][colIndex]
//      -= math::ad(thisJoint->getLocalJacobian(), temp_q_dq);
//  assert(mV_dq_dq[rowIndex][colIndex] == Eigen::Vector6d::Zero());
}

//==============================================================================
Eigen::Vector6d
BodyNodeDifferential::getBodyVelocityGradientWrtQ(
    std::size_t indexInSkeleton) const
{
  return mV_q.col(indexInSkeleton);
}

//==============================================================================
Eigen::Vector6d
BodyNodeDifferential::getBodyVelocityGradientWrtQ(
    const DegreeOfFreedom* withRespectTo) const
{
  const auto index = withRespectTo->getIndexInSkeleton();

  return getBodyVelocityGradientWrtQ(index);
}

//==============================================================================
BodyNodeDifferential::GradientMatrix
BodyNodeDifferential::getBodyVelocityGradientWrtQ(
    const Joint* withRespectTo) const
{
  const auto index = withRespectTo->getIndexInSkeleton(0);
  const auto numDofs = withRespectTo->getNumDofs();

  return mV_q.block(0, index, 6, numDofs);
}

//==============================================================================
BodyNodeDifferential::GradientMatrix
BodyNodeDifferential::getBodyVelocityGradientWrtQ() const
{
  return mV_q;
}

//==============================================================================
Eigen::Vector6d
BodyNodeDifferential::getBodyVelocityGradientWrtDQ(
    std::size_t indexInSkeleton) const
{
  return mV_dq.col(indexInSkeleton);
}

//==============================================================================
Eigen::Vector6d
BodyNodeDifferential::getBodyVelocityGradientWrtDQ(
    const DegreeOfFreedom* withRespectTo) const
{
  const auto index = withRespectTo->getIndexInSkeleton();

  return getBodyVelocityGradientWrtDQ(index);
}

//==============================================================================
BodyNodeDifferential::GradientMatrix
BodyNodeDifferential::getBodyVelocityGradientWrtDQ(
    const Joint* withRespectTo) const
{
  const auto index = withRespectTo->getIndexInSkeleton(0);
  const auto numDofs = withRespectTo->getNumDofs();

  return mV_dq.block(0, index, 6, numDofs);
}

//==============================================================================
BodyNodeDifferential::GradientMatrix
BodyNodeDifferential::getBodyVelocityGradientWrtDQ() const
{
  return mV_dq;
}

//==============================================================================
Eigen::VectorXd
BodyNodeDifferential::computeKineticEnergyGradientWrtPositions() const
{
  const BodyNode* thisBodyNode = mComposite;
  const Eigen::Matrix6d& G = thisBodyNode->getInertia().getSpatialTensor();
  const Eigen::Vector6d& V = thisBodyNode->getSpatialVelocity();

  return V.transpose() * G * mV_q;
}

//==============================================================================
Eigen::VectorXd
BodyNodeDifferential::computeKineticEnergyGradientWrtVelocities() const
{
  const BodyNode* thisBodyNode = mComposite;
  const Eigen::Matrix6d& G = thisBodyNode->getInertia().getSpatialTensor();
  const Eigen::Vector6d& V = thisBodyNode->getSpatialVelocity();

  return V.transpose() * G * mV_dq;
}

//==============================================================================
Eigen::VectorXd
BodyNodeDifferential::computeLagrangianGradientWrtPositions() const
{
  return computeKineticEnergyGradientWrtPositions();
}

//==============================================================================
Eigen::VectorXd
BodyNodeDifferential::computeLagrangianGradientWrtVelocities() const
{
  return computeKineticEnergyGradientWrtVelocities();
}

//==============================================================================
Eigen::MatrixXd
BodyNodeDifferential::computeKineticEnergyHessianWrtPositionsPositions() const
{

}

//==============================================================================
Eigen::MatrixXd
BodyNodeDifferential::computeKineticEnergyHessianWrtPositionsVelocities() const
{

}

//==============================================================================
Eigen::MatrixXd
BodyNodeDifferential::computeKineticEnergyHessianWrtVelocitiesVelocities() const
{

}

//==============================================================================
Eigen::MatrixXd
BodyNodeDifferential::computeLagrangianHessianWrtPositionsPositions() const
{
  return computeKineticEnergyHessianWrtPositionsPositions();
}

//==============================================================================
Eigen::MatrixXd
BodyNodeDifferential::computeLagrangianHessianWrtPositionsVelocities() const
{
  return computeKineticEnergyHessianWrtPositionsVelocities();
}

//==============================================================================
Eigen::MatrixXd
BodyNodeDifferential::computeLagrangianHessianWrtVelocitiesVelocities() const
{
  return computeKineticEnergyHessianWrtVelocitiesVelocities();
}

//==============================================================================
void BodyNodeDifferential::dirtySpatialVelocityDerivativeWrtPositions()
{

}

//==============================================================================
void BodyNodeDifferential::dirtySpatialVelocityDerivativeWrtVelocities()
{

}

//==============================================================================
void BodyNodeDifferential::print()
{
  std::cout << mV_q << std::endl;;
}

//==============================================================================
void BodyNodeDifferential::setComposite(common::Composite* newComposite)
{
  Base::setComposite(newComposite);

  const auto* bodyNode = dynamic_cast<BodyNode*>(newComposite);
  const auto skeleton = bodyNode->getSkeleton();
  const auto numDofs = skeleton->getNumDofs();

  assert(skeleton);

  mV_q.resize(6, numDofs);
  mV_dq.resize(6, numDofs);

  mV_q_q.resize(numDofs);
  mV_q_dq.resize(numDofs);
  mV_dq_dq.resize(numDofs);

  for (auto i = 0u; i < numDofs; ++i)
  {
    mV_q_q[i].resize(6, numDofs);
    mV_q_dq[i].resize(6, numDofs);
    mV_dq_dq[i].resize(6, numDofs);
  }

  // TODO(JS): These should be updated when the structure of skeleton is
  // modified. We might want to consider adding signal to skeleton for this.
}

} // namespace dynamics
} // namespace dart
