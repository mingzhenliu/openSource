#include "extensions/filters/http/router/config.h"

#include "envoy/config/filter/http/router/v2/router.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/config/filter_json.h"
#include "common/json/config_schemas.h"
#include "common/router/router.h"
#include "common/router/shadow_writer_impl.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace RouterFilter {

Http::FilterFactoryCb RouterFilterConfig::createFilterFactoryFromProtoTyped(
    const envoy::config::filter::http::router::v2::Router& proto_config,
    const std::string& stat_prefix, Server::Configuration::FactoryContext& context) {
  Router::FilterConfigSharedPtr filter_config(new Router::FilterConfig(
      stat_prefix, context, std::make_unique<Router::ShadowWriterImpl>(context.clusterManager()),
      proto_config));
//  增加stream 的filter
  return [filter_config](Http::FilterChainFactoryCallbacks& callbacks) -> void {
   //  ProdFilter ===  E:\envoy-master\source\common\router\router.cc
   // callbacks ==E:\envoy-master\source\common\http\conn_manager_impl.h
    callbacks.addStreamDecoderFilter(std::make_shared<Router::ProdFilter>(*filter_config));
  };
}

//E:\envoy-master\source\server\listener_manager_impl.cc  调用该函数，创建filter工厂
Http::FilterFactoryCb
RouterFilterConfig::createFilterFactory(const Json::Object& json_config,
                                        const std::string& stat_prefix,
                                        Server::Configuration::FactoryContext& context) {
  envoy::config::filter::http::router::v2::Router proto_config;
  Config::FilterJson::translateRouter(json_config, proto_config);
  return createFilterFactoryFromProtoTyped(proto_config, stat_prefix, context);
}

/**
 * Static registration for the router filter. @see RegisterFactory.
 */
static Registry::RegisterFactory<RouterFilterConfig,
                                 Server::Configuration::NamedHttpFilterConfigFactory>
    register_;

} // namespace RouterFilter
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
