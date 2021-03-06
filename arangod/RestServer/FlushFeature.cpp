////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#include "FlushFeature.h"

#include "ApplicationFeatures/ApplicationServer.h"
#include "Basics/ReadLocker.h"
#include "Basics/WriteLocker.h"
#include "Logger/Logger.h"
#include "ProgramOptions/ProgramOptions.h"
#include "ProgramOptions/Section.h"
#include "RestServer/DatabaseFeature.h"
#include "Utils/FlushThread.h"
#include "Utils/FlushTransaction.h"

using namespace arangodb;
using namespace arangodb::application_features;
using namespace arangodb::basics;
using namespace arangodb::options;

std::atomic<bool> FlushFeature::_isRunning(false);

FlushFeature::FlushFeature(ApplicationServer* server)
    : ApplicationFeature(server, "Flush"),
      _flushInterval(1000000) {
  setOptional(false);
  requiresElevatedPrivileges(false);
  startsAfter("WorkMonitor");
  startsAfter("StorageEngine");
  startsAfter("MMFilesLogfileManager");
  // TODO: must start after storage engine
}

void FlushFeature::collectOptions(std::shared_ptr<ProgramOptions> options) {
  options->addSection("server", "Server features");
  options->addOption(
      "--server.flush-interval",
      "interval (in microseconds) for flushing data",
      new UInt64Parameter(&_flushInterval));
}

void FlushFeature::validateOptions(std::shared_ptr<options::ProgramOptions> options) {
  if (_flushInterval < 1000) {
    // do not go below 1000 microseconds
    _flushInterval = 1000;
  }
}

void FlushFeature::prepare() {
}

void FlushFeature::start() {
  _flushThread.reset(new FlushThread(_flushInterval));
  DatabaseFeature* dbFeature = DatabaseFeature::DATABASE;
  dbFeature->registerPostRecoveryCallback(
    [this]() -> Result {
      if (!this->_flushThread->start()) {
        LOG_TOPIC(FATAL, Logger::FIXME) << "unable to start FlushThread";
        FATAL_ERROR_ABORT();
      }

      this->_isRunning.store(true);

      return {TRI_ERROR_NO_ERROR};
    }
  );
}

void FlushFeature::beginShutdown() {
  // pass on the shutdown signal
  if (_flushThread != nullptr) {
    _flushThread->beginShutdown();
  }
}

void FlushFeature::stop() {
  LOG_TOPIC(TRACE, arangodb::Logger::FIXME) << "stopping FlushThread";
  // wait until thread is fully finished

  if (_flushThread != nullptr) {
    while (_flushThread->isRunning()) {
      std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }

    _isRunning.store(false);
    _flushThread.reset();
  }
}

void FlushFeature::unprepare() {
  WRITE_LOCKER(locker, _callbacksLock);
  _callbacks.clear();
}

void FlushFeature::registerCallback(void* ptr, FlushFeature::FlushCallback const& cb) {
  WRITE_LOCKER(locker, _callbacksLock);
  _callbacks.emplace(ptr, std::move(cb));
}

bool FlushFeature::unregisterCallback(void* ptr) {
  WRITE_LOCKER(locker, _callbacksLock);

  auto it = _callbacks.find(ptr);
  if (it == _callbacks.end()) {
    return false;
  }

  _callbacks.erase(it);
  return true;
}

void FlushFeature::executeCallbacks() {
  std::vector<FlushTransactionPtr> transactions;

  READ_LOCKER(locker, _callbacksLock);

  transactions.reserve(_callbacks.size());

  // execute all callbacks. this will create as many transactions as
  // there are callbacks
  for (auto const& cb : _callbacks) {
    transactions.emplace_back(cb.second());
  }

  // TODO: make sure all data is synced


  // commit all transactions
  for (auto const& trx : transactions) {
    LOG_TOPIC(DEBUG, Logger::FIXME) << "commiting flush transaction '" << trx->name() << "'";

    Result res = trx->commit();

    if (!res.ok()) {
      LOG_TOPIC(ERR, Logger::FIXME) << "could not commit flush transaction '" << trx->name() << "': " << res.errorMessage();
    }
    // TODO: honor the commit results here
  }
}
