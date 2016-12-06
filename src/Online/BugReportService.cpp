#include "stormancer.h"
#include "BugReportService.h"

namespace Stormancer
{

	BugReportService::BugReportService(Stormancer::ScenePtr scene)
	{
		this->_scene = scene;
		this->_rpc = scene.lock()->dependencyResolver()->resolve<IRpcService>();
	}

	pplx::task<void> BugReportService::ReportBug(std::string category, std::string comments, std::vector<AttachedFileDescriptor> files)
	{
		auto metadata = BugReportMetadata();
		metadata.category = category;
		metadata.comment = comments;
		for (const auto& f : files)
		{
			auto attachedFile = BugReportAttachedFile();
			attachedFile.name = f.name;
			attachedFile.mimeType = f.contentType;
			attachedFile.length = f.contentLength;
			metadata.files.push_back(attachedFile);
		}
		return _rpc->rpcVoid_with_writer( "bugreporting.report", [&](bytestream* stream) {
		
			msgpack::pack(stream, metadata);
			for (const auto& f : files)
			{
				stream->write((char*)f.content, f.contentLength);
			}
		});
	}
}