// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Infrastracture and Nippon Telegraph and Telephone Corporation.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include <algorithm>
#include <cmath>
#include "lsh.hpp"
#include "../common/exception.hpp"
#include "../common/hash.hpp"

using namespace std;
using namespace pfi::data;
using namespace pfi::lang;
using namespace jubatus::storage;

namespace jubatus {
namespace recommender {

static const uint64_t DEFAULT_BASE_NUM = 64; // should be in config

lsh::lsh(uint64_t base_num){
}

lsh::lsh() : base_num_(DEFAULT_BASE_NUM){
}

lsh::~lsh(){
}

void lsh::similar_row(const sfv_t& query, vector<pair<string, float> > & ids, size_t ret_num) const{
  ids.clear();
  if (base_num_ == 0) return;

  bit_vector query_bv;
  calc_lsh_values(query, query_bv);
  row2lshvals_.similar_row(query_bv, ids, ret_num);
}

void lsh::clear(){
  orig_.clear();
  column2baseval_.clear();
  row2lshvals_.clear();
}

void lsh::clear_row(const string& id){
  orig_.remove_row(id);
  row2lshvals_.remove_row(id);
}

void lsh::calc_lsh_values(const sfv_t& sfv, bit_vector& bv) const{
  vector<float> lsh_vals(base_num_);
  for (size_t i = 0; i < sfv.size(); ++i){
    const string& column = sfv[i].first;
    float val = sfv[i].second;
    unordered_map<string, vector<float> >::const_iterator it = column2baseval_.find(column);
    if (it == column2baseval_.end()){
      continue;
    }
    const vector<float>& v = it->second;

    // assert(v.size() == base_num_);
    for (size_t j = 0; j < v.size(); ++j){
      lsh_vals[j] += v[j] * val;
    }  
  }

  bv.resize_and_clear(base_num_);
  for (size_t i = 0; i < lsh_vals.size(); ++i){
    if (lsh_vals[i] >= 0.f){
      bv.set_bit(i);
    }
  }
}

void lsh::generate_column_base(const string& column){
  // should use more clever hash
  if (column2baseval_.find(column) != column2baseval_.end()){
    return;
  }
  srand(hash_util::calc_string_hash(column));
  vector<float>& baseval = column2baseval_[column];
  baseval.resize(base_num_);
  for (uint64_t i = 0; i < base_num_; ++i){
    float v1 = (float)(rand()+1) / ((float)RAND_MAX + 1);
    float v2 = (float)(rand()+1) / ((float)RAND_MAX + 1);
    float z = sqrt(-2.f * log(v1)) * cos(2.f * M_PI * v2);
    baseval[i] = z;
  }
}

void lsh::update_row(const string& id, const sfv_diff_t& diff){
  for (size_t i = 0; i < diff.size(); ++i){
    generate_column_base(diff[i].first);
  }
  orig_.set_row(id, diff);
  sfv_t row;
  orig_.get_row(id, row);
  bit_vector bv;
  calc_lsh_values(row, bv);
  row2lshvals_.set_row(id, bv);
}

string lsh::type() const{
  return string("lsh");
}
bool lsh::save(std::ostream& os){
  pfi::data::serialization::binary_oarchive oa(os);
  oa << column2baseval_;
  oa << row2lshvals_;
  return true;
}
bool lsh::load(std::istream& is){
  pfi::data::serialization::binary_iarchive ia(is);
  ia >> column2baseval_;
  ia >> row2lshvals_;
  return true;
}
storage::recommender_storage_base* lsh::get_storage(){
  return reinterpret_cast<storage::recommender_storage_base*>(&row2lshvals_);
}
const storage::recommender_storage_base* lsh::get_const_storage()const{
  return reinterpret_cast<const storage::recommender_storage_base*>(&row2lshvals_);
}


} // namespace recommender
} // namespace jubatus

