#include "AnalyticsMapExport.h"
#include "DataWiseEditor.h"

void UAnalyticsHeatmap::WriteDensity(FVector2D Location, float Density)
{
	Location /= DensityResolution;
	Location = FVector2D(FMath::FloorToFloat(Location.X), FMath::FloorToFloat(Location.Y));
	Location *= DensityResolution;

	if (DensityData.Contains(Location))
	{
		DensityData[Location] = Density;
	}
	else
	{
		DensityData.Add(Location, Density);
	}
}

void UAnalyticsHeatmap::AddDensity(FVector2D Location, float Density)
{
	Location /= DensityResolution;
	Location = FVector2D(FMath::FloorToFloat(Location.X), FMath::FloorToFloat(Location.Y));
	Location *= DensityResolution;

	if (DensityData.Contains(Location)) 
	{
		DensityData[Location] += Density;
	}
	else
	{
		DensityData.Add(Location, Density);
	}
}

void UAnalyticsHeatmap::Setup(FVector2D resolution)
{
	DensityResolution = resolution;
}

FString UAnalyticsHeatmap::Export(FString prefix)
{
	FIntPoint Size = ParentMap->GetSize();
	FVector2D WorldSize = ParentMap->GetWorldSize();

	BitmapRenderer rast(Size.X, Size.Y);
	rast.Clear(FColor(0, 0, 0, 0));

	float density_min = 0;
	float density_max = 0;

	for (auto tile : DensityData)
	{
		if (density_min == 0 && density_max == 0)
		{
			density_min = tile.Value;
			density_max = tile.Value;
		}

		density_min = FMath::Min(density_min, tile.Value);
		density_max = FMath::Max(density_max, tile.Value);
	}

	for (auto tile : DensityData)
	{
		FVector2D pos = tile.Key;
		pos /= WorldSize;
		pos += FVector2D(.5f, .5f);
		pos *= FVector2D(Size);

		float progress = 1.0f;
		if (density_min != density_max) { progress = (tile.Value - density_min) / (density_max - density_min); }

		FColor color;
		if (progress <= .5f)
		{
			color = FColor(0, progress * 2 * 255, (1.0f - progress * 2.0f) * 255, 200);
		}
		else
		{
			color = FColor((progress - 0.5f) * 2 * 255, (1.0f - (progress - 0.5f) * 2) * 255, 0, 200);
		}


		FIntPoint position = FIntPoint(FMath::FloorToInt(pos.X), FMath::FloorToInt(pos.Y));

		FVector2D tile_size_float = (DensityResolution / WorldSize) * Size;

		FVector2D end_corner = pos + tile_size_float;
		FIntPoint tile_size = FIntPoint(FMath::FloorToInt(end_corner.X) - position.X, FMath::FloorToInt(end_corner.Y) - position.Y);

		rast.DrawRectangle(position, tile_size, color);
	}

	FString directory = FPaths::ProjectDir() + local_data_output;
	FString file = prefix + ExportName + ".png";
	FString path = directory + file;
	rast.Export(path);

	return file;
}

void UAnalyticsMap::Setup(int Width, int Height, UTexture2D* Background, FVector2D WorldSize, FVector2D WorldCenter) 
{
	Size = { Width, Height };
	this->Background = Background;
	this->WorldSize = WorldSize;
	this->WorldCenter = WorldCenter;
}

void UAnalyticsMap::ExportData(FString prefix, uint32 unique_id, FString& output_data, FString& output_graph)
{
	BitmapRenderer rast(Size.X, Size.Y);
	rast.Clear(FColor::Black);

	rast.DrawImage({ 0, 0 }, Size, Background);

	FString directory = FPaths::ProjectDir() + local_data_output;
	FString file = prefix + ExportName + ".png";
	FString path = directory + file;
	rast.Export(path);

	output_graph += "<h1>" + ExportName + "</h1>";
	output_graph += "<div class='map_parent'>";
	output_graph += "<button class = 'btn_zoomin'> </button><button class='btn_zoomout'> </button>";
	output_graph += "<div class='map_holder'>";
	output_graph += "<div class='map_layer'>";
	output_graph += "<img src='" + file + "' draggable='false'>";

	for (UAnalyticsMapChannel* channel : channels)
	{
		FString file = channel->Export(prefix);
		output_graph += "<img src='" + file + "' class='map_image' draggable='false'>";
	}
	output_graph += "</div></div>";

	output_graph += "<script>var map_root" + FString::FromInt(unique_id) + " = document.currentScript.parentNode;var map_list" + FString::FromInt(unique_id) + " = map_root" + FString::FromInt(unique_id) + ".getElementsByClassName('map_layer')[0]; var map" + FString::FromInt(unique_id) + "_zoomlevel = 0; var map" + FString::FromInt(unique_id) + "_center = [.5, .5]; var map" + FString::FromInt(unique_id) + "_panning = false; var map" + FString::FromInt(unique_id) + "_lastmouseposition = [0, 0]; map" + FString::FromInt(unique_id) + "_zoom(); map_list0.addEventListener('mousedown', (event) => { map" + FString::FromInt(unique_id) + "_panning = true; map" + FString::FromInt(unique_id) + "_lastmouseposition = [event.clientX, event.clientY]; }); map_list0.addEventListener('mouseup', (event) => { map" + FString::FromInt(unique_id) + "_panning = false; }); map_list0.addEventListener('mousemove', (event) => { if(map" + FString::FromInt(unique_id) + "_panning) { var delta_pos = [event.clientX - map" + FString::FromInt(unique_id) + "_lastmouseposition[0], event.clientY - map" + FString::FromInt(unique_id) + "_lastmouseposition[1]]; map" + FString::FromInt(unique_id) + "_lastmouseposition = [event.clientX, event.clientY]; delta_pos[0] /= map_list0.clientWidth; delta_pos[1] /= map_list0.clientHeight;  var scale = 1; for(var i=0;i < map" + FString::FromInt(unique_id) + "_zoomlevel;i+=1){ scale += scale; }  delta_pos[0] /= scale; delta_pos[1] /= scale;  map" + FString::FromInt(unique_id) + "_center[0]-=delta_pos[0]; map" + FString::FromInt(unique_id) + "_center[1]-=delta_pos[1];  map" + FString::FromInt(unique_id) + "_zoom(); } }); function map" + FString::FromInt(unique_id) + "_zoom(){ map" + FString::FromInt(unique_id) + "_center[0] = Math.max(0, Math.min(1, map" + FString::FromInt(unique_id) + "_center[0])); map" + FString::FromInt(unique_id) + "_center[1] = Math.max(0, Math.min(1, map" + FString::FromInt(unique_id) + "_center[1])); var scale = 1; for(var i=0;i < map" + FString::FromInt(unique_id) + "_zoomlevel;i+=1){ scale += scale; } map_list" + FString::FromInt(unique_id) + ".style.transform = 'scale(' + scale + ')';  var x, y; x = map" + FString::FromInt(unique_id) + "_center[0] - 0.5; y = map" + FString::FromInt(unique_id) + "_center[1] - 0.5; x *= -map_list" + FString::FromInt(unique_id) + ".clientWidth; y *= -map_list" + FString::FromInt(unique_id) + ".clientHeight;  map_list" + FString::FromInt(unique_id) + ".style.transform += 'translate(' + x + 'px, ' + y + 'px)'; }  map_root" + FString::FromInt(unique_id) + ".getElementsByClassName('btn_zoomin')[0].onclick = function(){ map" + FString::FromInt(unique_id) + "_zoomlevel += 1; map" + FString::FromInt(unique_id) + "_zoom(); }; map_root" + FString::FromInt(unique_id) + ".getElementsByClassName('btn_zoomout')[0].onclick = function(){ map" + FString::FromInt(unique_id) + "_zoomlevel = Math.max(0, map" + FString::FromInt(unique_id) + "_zoomlevel-1); map" + FString::FromInt(unique_id) + "_zoom(); };</script>";

	output_graph += "<div class='layer_list'>";

	for (int i = 0; i < channels.Num(); i++) {
		output_graph += "<label class='layer_switch'><input type='checkbox' checked><span class='slider'></span></label><span>" + channels[i]->ChannelName + "</span><br><br>";
		output_graph += "<script type='text/javascript'>map_root" + FString::FromInt(unique_id) + ".getElementsByClassName('layer_list')[0].getElementsByClassName('layer_switch')[" + FString::FromInt(i) + "].getElementsByTagName('input')[0].addEventListener('change', (event) => {map_list" + FString::FromInt(unique_id) + ".getElementsByClassName('map_image')[" + FString::FromInt(i) + "].style.visibility = event.target.checked ? 'visible' : 'hidden';});</script>";
	}
	output_graph += "</div></div><br style='clear:both'/>";

	for (UAnalyticsMapChannel* channel : channels)
	{
		channel->RemoveFromRoot();
		channel->ConditionalBeginDestroy();
	}
	channels.Empty();
}

UAnalyticsHeatmap* UAnalyticsMap::AddDensityChannel(FString ChannelName, FVector2D DensityResolution)
{
	UAnalyticsHeatmap* channel = NewObject<UAnalyticsHeatmap>();
	channel->AddToRoot();
	channel->Setup(DensityResolution);
	channel->ChannelName = ChannelName;
	channel->ExportName = ExportName + "_" + ChannelName;
	channel->ParentMap = this;
	channels.Add(channel);
	return channel;
}
