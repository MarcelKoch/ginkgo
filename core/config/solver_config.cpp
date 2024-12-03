// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#include "core/config/solver_config.hpp"

#include <ginkgo/core/base/exception_helpers.hpp>
#include <ginkgo/core/config/config.hpp>
#include <ginkgo/core/config/registry.hpp>
#include <ginkgo/core/solver/bicg.hpp>
#include <ginkgo/core/solver/bicgstab.hpp>
#include <ginkgo/core/solver/cb_gmres.hpp>
#include <ginkgo/core/solver/cg.hpp>
#include <ginkgo/core/solver/cgs.hpp>
#include <ginkgo/core/solver/direct.hpp>
#include <ginkgo/core/solver/fcg.hpp>
#include <ginkgo/core/solver/gcr.hpp>
#include <ginkgo/core/solver/gmres.hpp>
#include <ginkgo/core/solver/idr.hpp>
#include <ginkgo/core/solver/ir.hpp>
#include <ginkgo/core/solver/multigrid.hpp>
#include <ginkgo/core/solver/triangular.hpp>

#include "core/config/config_helper.hpp"
#include "core/config/dispatch.hpp"
#include "core/config/parse_macro.hpp"


namespace gko {
namespace config {


GKO_PARSE_VALUE_TYPE_WITH_HALF(Cg, gko::solver::Cg);
GKO_PARSE_VALUE_TYPE_WITH_HALF(Bicg, gko::solver::Bicg);
GKO_PARSE_VALUE_TYPE_WITH_HALF(Bicgstab, gko::solver::Bicgstab);
GKO_PARSE_VALUE_TYPE_WITH_HALF(Cgs, gko::solver::Cgs);
GKO_PARSE_VALUE_TYPE_WITH_HALF(Fcg, gko::solver::Fcg);
GKO_PARSE_VALUE_TYPE_WITH_HALF(Ir, gko::solver::Ir);
GKO_PARSE_VALUE_TYPE_WITH_HALF(Idr, gko::solver::Idr);
GKO_PARSE_VALUE_TYPE_WITH_HALF(Gcr, gko::solver::Gcr);
GKO_PARSE_VALUE_TYPE_WITH_HALF(Gmres, gko::solver::Gmres);
GKO_PARSE_VALUE_TYPE(CbGmres, gko::solver::CbGmres);
GKO_PARSE_VALUE_AND_INDEX_TYPE_WITH_HALF(Direct,
                                         gko::experimental::solver::Direct);
GKO_PARSE_VALUE_AND_INDEX_TYPE_WITH_HALF(LowerTrs, gko::solver::LowerTrs);
GKO_PARSE_VALUE_AND_INDEX_TYPE_WITH_HALF(UpperTrs, gko::solver::UpperTrs);


template <>
deferred_factory_parameter<gko::LinOpFactory>
parse<LinOpFactoryType::Multigrid>(const pnode& config, const registry& context,
                                   const gko::config::type_descriptor& td)
{
    auto updated = update_type(config, td);
    return solver::Multigrid::parse(config, context, updated);
}


}  // namespace config
}  // namespace gko
