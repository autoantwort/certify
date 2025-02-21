#ifndef BOOST_CERTIFY_IMPL_RFC2818_VERIFICATION_IPP
#define BOOST_CERTIFY_IMPL_RFC2818_VERIFICATION_IPP

#include <boost/certify/https_verification.hpp>

#include <boost/asio/ip/address.hpp>
#include <openssl/x509v3.h>

#include <memory>

namespace boost
{
namespace certify
{
namespace detail
{

extern "C" BOOST_CERTIFY_DECL int
verify_server_certificates(::X509_STORE_CTX* ctx, void* hostname) noexcept
{
    std::unique_ptr<char[]> host;
    host.reset((char*)hostname);
    if (::X509_verify_cert(ctx) == 1)
        return true;

    auto const err = ::X509_STORE_CTX_get_error(ctx);
    if ((err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN ||
         err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT ||
         err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY) &&
        detail::verify_certificate_chain(ctx, std::move(host)))
    {
        ::X509_STORE_CTX_set_error(ctx, X509_V_OK);
        return true;
    }

    return false;
}

void
set_server_hostname(::SSL* handle, string_view hostname, system::error_code& ec)
{
    auto* param = ::SSL_get0_param(handle);
    ::X509_VERIFY_PARAM_set_hostflags(param,
                                      X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    if (!X509_VERIFY_PARAM_set1_host(param, hostname.data(), hostname.size()))
        ec = {static_cast<int>(::ERR_get_error()),
              asio::error::get_ssl_category()};
    else
        ec = {};
}

} // namespace detail

BOOST_CERTIFY_DECL void
enable_native_https_server_verification(asio::ssl::context& context, string_view hostname)
{
    char* host = new char[hostname.size() + 1];
    std::copy(hostname.begin(), hostname.end(), host);
    host[hostname.size()] = 0;
    ::SSL_CTX_set_cert_verify_callback(
      context.native_handle(), &detail::verify_server_certificates, host);
}

BOOST_CERTIFY_DECL void
enable_native_https_server_verification(asio::ssl::context& context, const char * hostname)
{
    char * dup = nullptr;
    if (hostname) {
        dup = new char[strlen(hostname) + 1];
        std::strcpy(dup, hostname);
    }
    ::SSL_CTX_set_cert_verify_callback(
        context.native_handle(), &detail::verify_server_certificates, dup);
}

} // namespace certify
} // namespace boost

#endif // BOOST_CERTIFY_IMPL_RFC2818_VERIFICATION_IPP
