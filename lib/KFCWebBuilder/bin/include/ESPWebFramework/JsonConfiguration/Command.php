<?php

/**
 * Code generated by json2php. DO NOT EDIT.
 * Generator at 2018-12-29 19:28:11
 */

namespace ESPWebFramework\JsonConfiguration;

class Command extends Converter\Command
{
    public const TYPE_COMMAND = 0;
    public const TYPE_PHP_CLASS = 1;
    public const TYPE_PHP_EVAL = 2;
    public const TYPE_INTERNAL_COMMAND = 3;

    /** @var \ESPWebFramework\JsonConfiguration\Type\Enum|null */
    private $type;

    /**
     * @var string|null
     * @validator \ESPWebFramework\FileUtils::realpath
     * @validator file_exists
     * @validator is_file
     */
    private $includeFile;

    /** @var string|null */
    private $className;

    /** @var string|null */
    private $code;

    /** @var string|null */
    private $command;

    /**
     * @return \ESPWebFramework\JsonConfiguration\Type\Enum|null
     */
    public function getType(): ?Type\Enum
    {
        return $this->type;
    }

    /**
     * @return string|null
     */
    public function getIncludeFile(): ?string
    {
        return $this->includeFile;
    }

    /**
     * @return string|null
     */
    public function getClassName(): ?string
    {
        return $this->className;
    }

    /**
     * @return string|null
     */
    public function getCode(): ?string
    {
        return $this->code;
    }

    /**
     * @return string|null
     */
    public function getCommand(): ?string
    {
        return $this->command;
    }
}
