////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
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

#ifndef ARANGOD_AQL_COLLECTION_SCANNER_H
#define ARANGOD_AQL_COLLECTION_SCANNER_H 1

#include "Basics/Common.h"
#include "Indexes/IndexElement.h"

namespace arangodb {
class ManagedDocumentResult;
struct OperationCursor;
class Transaction;

namespace aql {

class CollectionScanner {
 public:
  CollectionScanner(arangodb::Transaction*, ManagedDocumentResult*, std::string const&, bool);

  ~CollectionScanner();

  void scan(std::vector<IndexLookupResult>& result, size_t batchSize);

  void reset();

  /// @brief forwards the cursor n elements. Does not read the data.
  ///        Will at most forward to the last element.
  ///        In the second parameter we add how many elements are
  ///        really skipped
  int forward(size_t, uint64_t&);

 private:
  std::unique_ptr<OperationCursor> _cursor;
};
}
}

#endif