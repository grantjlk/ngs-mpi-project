package com.lsc

import NetGraphAlgebraDefs.NetGraph
import NetGraphAlgebraDefs.Action
import Utilz.CreateLogger
import scala.jdk.CollectionConverters.*
import scala.jdk.OptionConverters.* // This provides the .toScala extension
import java.io.FileWriter

object Export:
  val logger = CreateLogger(classOf[Export.type])

  def main(args: Array[String]): Unit =
    if args.length < 2 then
      logger.error("Usage: Export <full-path-to-input.ngs> <full-path-to-output.json>")
      sys.exit(1)

    val inputPath = args(0)
    val outputPath = args(1)

    logger.info(s"Loading graph from $inputPath")
    val g = NetGraph.load(fileName = inputPath, dir = "")

    if g.isEmpty then
      logger.error("Failed to load graph - check the file path")
      sys.exit(1)

    val graph = g.get
    val nodes = graph.sm.nodes().asScala.toList.sortBy(_.id)
    val edges = graph.sm.edges().asScala.toList

    logger.info(s"Loaded ${nodes.size} nodes and ${edges.size} edges")

    val idMap: Map[Int, Int] = nodes.map(_.id).zipWithIndex.toMap

    val nodesJson = nodes.map { n =>
      s"""    {"id": ${idMap(n.id)}}"""
    }.mkString(",\n")

    val edgesJson = edges.flatMap { e =>
    val srcOpt = idMap.get(e.nodeU().id)
    val dstOpt = idMap.get(e.nodeV().id)
    val actionOpt = graph.sm.edgeValue(e.nodeU(), e.nodeV()).toScala
    (srcOpt, dstOpt, actionOpt) match
      case (Some(src), Some(dst), Some(action)) =>
        val weight = math.max(1, (action.cost * 20).toInt)
        // add both directions to make graph undirected
        List(
          s"""    {"src": $src, "dst": $dst, "weight": $weight}""",
          s"""    {"src": $dst, "dst": $src, "weight": $weight}"""
        )
      case _ =>
        logger.warn(s"Skipping edge with unmapped node ids or missing action")
        List.empty
  }.mkString(",\n")

    val json =
      s"""{
  "num_nodes": ${nodes.size},
  "nodes": [
$nodesJson
  ],
  "edges": [
$edgesJson
  ]
}"""

    val fw = new FileWriter(outputPath)
    fw.write(json)
    fw.close()
    logger.info(s"Successfully exported graph to $outputPath")

end Export