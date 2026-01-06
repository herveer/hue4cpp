#include "hue4cpp/http_client.h"
#include <cpr/cpr.h>

namespace hue4cpp {

// HttpClient::Impl definition
class HttpClient::Impl {
public:
    std::chrono::milliseconds timeout;
    bool verify_ssl;
    
    Impl() : timeout(10000), verify_ssl(true) {}
    
    void configureSession(cpr::Session& session) {
        session.SetTimeout(timeout);
        if (!verify_ssl) {
            session.SetVerifySsl(cpr::VerifySsl{false});
        }
    }
    
    HttpResponse convertResponse(const cpr::Response& response) {
        HttpResponse result;
        result.status_code = static_cast<int>(response.status_code);
        result.body = response.text;
        
        // Convert headers
        for (const auto& [key, value] : response.header) {
            result.headers[key] = value;
        }
        
        // Set error message if request failed
        if (response.error.code != cpr::ErrorCode::OK) {
            result.error_message = response.error.message;
        }
        
        return result;
    }
};

// HttpClient implementation
HttpClient::HttpClient() : pImpl(std::make_unique<Impl>()) {}

HttpClient::~HttpClient() = default;

HttpClient::HttpClient(HttpClient&&) noexcept = default;
HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;

void HttpClient::setTimeout(std::chrono::milliseconds timeout) {
    pImpl->timeout = timeout;
}

void HttpClient::setVerifySsl(bool verify) {
    pImpl->verify_ssl = verify;
}

HttpResponse HttpClient::get(const std::string& url,
                              const std::map<std::string, std::string>& headers) {
    cpr::Session session;
    pImpl->configureSession(session);
    session.SetUrl(cpr::Url{url});
    
    // Set headers
    if (!headers.empty()) {
        cpr::Header cpr_headers;
        for (const auto& [key, value] : headers) {
            cpr_headers[key] = value;
        }
        session.SetHeader(cpr_headers);
    }
    
    cpr::Response response = session.Get();
    return pImpl->convertResponse(response);
}

HttpResponse HttpClient::post(const std::string& url,
                               const std::string& body,
                               const std::map<std::string, std::string>& headers) {
    cpr::Session session;
    pImpl->configureSession(session);
    session.SetUrl(cpr::Url{url});
    session.SetBody(cpr::Body{body});
    
    // Set default Content-Type if not provided
    cpr::Header cpr_headers;
    bool has_content_type = false;
    for (const auto& [key, value] : headers) {
        cpr_headers[key] = value;
        if (key == "Content-Type" || key == "content-type") {
            has_content_type = true;
        }
    }
    if (!has_content_type) {
        cpr_headers["Content-Type"] = "application/json";
    }
    session.SetHeader(cpr_headers);
    
    cpr::Response response = session.Post();
    return pImpl->convertResponse(response);
}

HttpResponse HttpClient::put(const std::string& url,
                              const std::string& body,
                              const std::map<std::string, std::string>& headers) {
    cpr::Session session;
    pImpl->configureSession(session);
    session.SetUrl(cpr::Url{url});
    session.SetBody(cpr::Body{body});
    
    // Set default Content-Type if not provided
    cpr::Header cpr_headers;
    bool has_content_type = false;
    for (const auto& [key, value] : headers) {
        cpr_headers[key] = value;
        if (key == "Content-Type" || key == "content-type") {
            has_content_type = true;
        }
    }
    if (!has_content_type) {
        cpr_headers["Content-Type"] = "application/json";
    }
    session.SetHeader(cpr_headers);
    
    cpr::Response response = session.Put();
    return pImpl->convertResponse(response);
}

HttpResponse HttpClient::del(const std::string& url,
                              const std::map<std::string, std::string>& headers) {
    cpr::Session session;
    pImpl->configureSession(session);
    session.SetUrl(cpr::Url{url});
    
    // Set headers
    if (!headers.empty()) {
        cpr::Header cpr_headers;
        for (const auto& [key, value] : headers) {
            cpr_headers[key] = value;
        }
        session.SetHeader(cpr_headers);
    }
    
    cpr::Response response = session.Delete();
    return pImpl->convertResponse(response);
}

} // namespace hue4cpp
