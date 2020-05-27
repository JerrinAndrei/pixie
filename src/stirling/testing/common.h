#pragma once

#include <memory>
#include <string>
#include <vector>

#include <absl/strings/substitute.h>

#include "src/common/testing/testing.h"
#include "src/shared/metadata/base_types.h"
#include "src/shared/types/column_wrapper.h"

namespace pl {

namespace types {

// Teach gMock to print out a SharedColumnWrapper.
inline std::ostream& operator<<(std::ostream& os, const ::pl::types::SharedColumnWrapper& col) {
  os << absl::Substitute("size=$0", col->Size());
  return os;
}

}  // namespace types

namespace stirling {
namespace testing {

MATCHER_P(ColWrapperSizeIs, size, absl::Substitute("is a ColumnWrapper having $0 elements", size)) {
  return arg->Size() == static_cast<size_t>(size);
}

MATCHER(ColWrapperIsEmpty, "is an empty ColumnWrapper") { return arg->Empty(); }

std::vector<size_t> FindRecordIdxMatchesPID(const types::ColumnWrapperRecordBatch& record,
                                            int upid_column_idx, int pid) {
  std::vector<size_t> res;
  for (size_t i = 0; i < record[upid_column_idx]->Size(); ++i) {
    md::UPID upid(record[upid_column_idx]->Get<types::UInt128Value>(i).val);
    if (upid.pid() == static_cast<uint64_t>(pid)) {
      res.push_back(i);
    }
  }
  return res;
}

std::shared_ptr<types::ColumnWrapper> SelectColumnWrapperRows(const types::ColumnWrapper& src,
                                                              const std::vector<size_t>& indices) {
  auto out = types::ColumnWrapper::Make(src.data_type(), 0);
  out->Reserve(indices.size());

#define TYPE_CASE(_dt_)                                                 \
  for (int idx : indices) {                                             \
    out->Append(src.Get<types::DataTypeTraits<_dt_>::value_type>(idx)); \
  }
  PL_SWITCH_FOREACH_DATATYPE(src.data_type(), TYPE_CASE);
#undef TYPE_CASE

  return out;
}

types::ColumnWrapperRecordBatch SelectRecordBatchRows(const types::ColumnWrapperRecordBatch& src,
                                                      const std::vector<size_t>& indices) {
  types::ColumnWrapperRecordBatch out;
  for (const auto& col : src) {
    out.push_back(SelectColumnWrapperRows(*col, indices));
  }

  return out;
}

types::ColumnWrapperRecordBatch FindRecordsMatchingPID(const types::ColumnWrapperRecordBatch& rb,
                                                       int upid_column_idx, int pid) {
  std::vector<size_t> indices = FindRecordIdxMatchesPID(rb, upid_column_idx, pid);
  return SelectRecordBatchRows(rb, indices);
}

// Note the index is column major, so it comes before row_idx.
template <typename TValueType>
const TValueType& AccessRecordBatch(const types::ColumnWrapperRecordBatch& record_batch,
                                    int column_idx, int row_idx) {
  return record_batch[column_idx]->Get<TValueType>(row_idx);
}

template <>
const std::string& AccessRecordBatch<std::string>(
    const types::ColumnWrapperRecordBatch& record_batch, int column_idx, int row_idx) {
  return record_batch[column_idx]->Get<types::StringValue>(row_idx);
}

}  // namespace testing
}  // namespace stirling
}  // namespace pl
