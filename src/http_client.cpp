#include "hue4cpp/http_client.h"
#include <cpr/cpr.h>

namespace hue4cpp {

namespace {
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
    
    void configureSession(cpr::Session& session, std::chrono::milliseconds timeout, bool verify_ssl) {
        session.SetTimeout(timeout);
        if (!verify_ssl) {
            session.SetVerifySsl(cpr::VerifySsl{false});
        }
    }
}

// HttpClient implementation
HttpClient::HttpClient() : timeout_(10000), verify_ssl_(true) {}

void HttpClient::setTimeout(std::chrono::milliseconds timeout) {
    timeout_ = timeout;
}

void HttpClient::setVerifySsl(bool verify) {
    verify_ssl_ = verify;
}

bool HttpClient::hasContentType(const std::map<std::string, std::string>& headers) const {
    for (const auto& [key, value] : headers) {
        if (key == "Content-Type" || key == "content-type") {
            return true;
        }
    }
    return false;
}

HttpResponse HttpClient::get(const std::string& url,
                              const std::map<std::string, std::string>& headers) {
    cpr::Session session;
    configureSession(session, timeout_, verify_ssl_);
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
    return convertResponse(response);
}

HttpResponse HttpClient::post(const std::string& url,
                               const std::string& body,
                               const std::map<std::string, std::string>& headers) {
    cpr::Session session;
    configureSession(session, timeout_, verify_ssl_);
    session.SetUrl(cpr::Url{url});
    session.SetBody(cpr::Body{body});
    
    // Set default Content-Type if not provided
    cpr::Header cpr_headers;
    for (const auto& [key, value] : headers) {
        cpr_headers[key] = value;
    }
    if (!hasContentType(headers)) {
        cpr_headers["Content-Type"] = "application/json";
    }
    session.SetHeader(cpr_headers);
    
    cpr::Response response = session.Post();
    return convertResponse(response);
}

HttpResponse HttpClient::put(const std::string& url,
                              const std::string& body,
                              const std::map<std::string, std::string>& headers) {
    cpr::Session session;
    configureSession(session, timeout_, verify_ssl_);
    session.SetUrl(cpr::Url{url});
    session.SetBody(cpr::Body{body});
    
    // Set default Content-Type if not provided
    cpr::Header cpr_headers;
    for (const auto& [key, value] : headers) {
        cpr_headers[key] = value;
    }
    if (!hasContentType(headers)) {
        cpr_headers["Content-Type"] = "application/json";
    }
    session.SetHeader(cpr_headers);
    
    cpr::Response response = session.Put();
    return convertResponse(response);
}

HttpResponse HttpClient::del(const std::string& url,
                              const std::map<std::string, std::string>& headers) {
    cpr::Session session;
    configureSession(session, timeout_, verify_ssl_);
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
    return convertResponse(response);
}

} // namespace hue4cpp
