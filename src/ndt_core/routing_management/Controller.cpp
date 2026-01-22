#include "ndt_core/routing_management/Controller.hpp"
#include "ndt_core/routing_management/FlowRoutingManager.hpp"

Controller::Controller(std::shared_ptr<FlowRoutingManager> flowRoutingManager)
    : m_flowRoutingManager(std::move(flowRoutingManager)),
      dispatcher_(
          // SenderFn: batch send using your existing flow manager
          [this](const std::vector<FlowJob>& batch) {
              for (const auto& job : batch)
              {
                  switch (job.op)
                  {
                  case FlowOp::Install:
                      m_flowRoutingManager->installAnEntry(job.dpid,
                                                           job.priority,
                                                           job.match,
                                                           job.actions,
                                                           job.idleTimeout);
                      break;
                  case FlowOp::Modify:
                      m_flowRoutingManager->modifyAnEntry(job.dpid,
                                                          job.priority,
                                                          job.match,
                                                          job.actions);
                      break;
                  case FlowOp::Delete:
                      m_flowRoutingManager->deleteAnEntry(job.dpid, job.match, job.priority);
                      break;
                  }
              }
              // (Optional) fence/Barrier here if your southbound supports it
          },
          /*burstSize*/ 2000,
          /*fencePerBurst*/ false)
{
    dispatcher_.start();
}

Controller::~Controller()
{
    dispatcher_.stop();
}
